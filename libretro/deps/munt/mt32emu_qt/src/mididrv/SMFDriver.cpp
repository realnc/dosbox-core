/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SMFDriver.h"

#include <QtCore>
#include <QFileDialog>
#include <QMessageBox>

#include "../MidiSession.h"

static const MasterClockNanos MAX_SLEEP_TIME = 200 * MasterClock::NANOS_PER_MILLISECOND;

static void sendAllSoundOff(SynthRoute *synthRoute, bool resetAllControllers, bool discardMidiBuffers) {
	if (synthRoute->getState() != SynthRouteState_OPEN) return;
	if (discardMidiBuffers) {
		synthRoute->discardMidiBuffers();
	} else {
		synthRoute->flushMIDIQueue();
	}
	for (quint8 i = 0; i < 16; i++) {
		// All notes off
		quint32 msg = 0x7FB0 | i;
		synthRoute->playMIDIShortMessageNow(msg);
		if (resetAllControllers) {
			// Reset all controllers
			msg = 0x79B0 | i;
			synthRoute->playMIDIShortMessageNow(msg);
		}
	}
}

SMFProcessor::SMFProcessor(SMFDriver *useSMFDriver) : driver(useSMFDriver) {
}

void SMFProcessor::start(QString useFileName) {
	driver->stopProcessing = false;
	driver->pauseProcessing = false;
	driver->bpmUpdate = 0;
	driver->fastForwardingFactor = 0;
	driver->seekPosition = -1;
	fileName = useFileName;
	if (!parser.parse(fileName)) {
		qDebug() << "SMFDriver: Error parsing MIDI file:" << fileName;
		QMessageBox::warning(NULL, "Error", "Error encountered while loading MIDI file");
		emit driver->playbackFinished();
		return;
	}
	QThread::start(QThread::TimeCriticalPriority);
}

void SMFProcessor::run() {
	MidiSession *session = driver->createMidiSession(QFileInfo(fileName).fileName());
	SynthRoute *synthRoute = session->getSynthRoute();
	bool paused = false;
	const QMidiEventList &midiEvents = parser.getMIDIEvents();
	midiTick = parser.getMidiTick();
	quint32 totalSeconds = estimateRemainingTime(midiEvents, 0);
	MasterClockNanos startNanos = MasterClock::getClockNanos();
	MasterClockNanos currentNanos = startNanos;
	for (int currentEventIx = 0; currentEventIx < midiEvents.count(); currentEventIx++) {
		currentNanos += midiEvents.at(currentEventIx).getTimestamp() * midiTick;
		while (!driver->stopProcessing && synthRoute->getState() == SynthRouteState_OPEN) {
			uint bpmUpdate = uint(driver->bpmUpdate.fetchAndStoreRelaxed(0));
			if (bpmUpdate > 0) {
				midiTick = parser.getMidiTick(MidiParser::MICROSECONDS_PER_MINUTE / bpmUpdate);
				totalSeconds = (currentNanos - startNanos) / MasterClock::NANOS_PER_SECOND + estimateRemainingTime(midiEvents, currentEventIx + 1);
			}
			MasterClockNanos nanosNow = MasterClock::getClockNanos();
			if (driver->pauseProcessing) {
				if (!paused) {
					paused = true;
					sendAllSoundOff(synthRoute, false, false);
				}
				usleep(MAX_SLEEP_TIME / MasterClock::NANOS_PER_MICROSECOND);
				MasterClockNanos delay = MasterClock::getClockNanos() - nanosNow;
				startNanos += delay;
				currentNanos += delay;
				continue;
			}
			if (paused) paused = false;
			int seekPosition = driver->seekPosition.fetchAndStoreRelaxed(-1);
			if (seekPosition > -1) {
				MasterClockNanos seekNanosSinceStart = totalSeconds * seekPosition * MasterClock::NANOS_PER_MILLISECOND;
				MasterClockNanos currentNanosSinceStart = currentNanos - startNanos;
				MasterClockNanos lastEventNanosSinceStart = currentNanosSinceStart - midiEvents.at(currentEventIx).getTimestamp() * midiTick;
				bool rewind;
				if (seekNanosSinceStart < lastEventNanosSinceStart || seekNanosSinceStart == 0) {
					midiTick = parser.getMidiTick();
					emit driver->tempoUpdated(0);
					currentEventIx = 0;
					currentNanosSinceStart = midiEvents.at(currentEventIx).getTimestamp() * midiTick;
					rewind = true;
				} else {
					rewind = false;
				}
				sendAllSoundOff(synthRoute, rewind, rewind);
				seek(synthRoute, midiEvents, currentEventIx, currentNanosSinceStart, seekNanosSinceStart);
				nanosNow = MasterClock::getClockNanos();
				startNanos = nanosNow - seekNanosSinceStart;
				currentNanos = currentNanosSinceStart + startNanos;
			}
			emit driver->playbackTimeChanged(nanosNow - startNanos, totalSeconds);
			MasterClockNanos delay = currentNanos - nanosNow;
			uint fastForwardingFactor = driver->fastForwardingFactor;
			if (fastForwardingFactor > 1) {
				MasterClockNanos timeShift = delay - (delay / fastForwardingFactor);
				delay -= timeShift;
				currentNanos -= timeShift;
				startNanos -= timeShift;
			}
			if (delay < MasterClock::NANOS_PER_MILLISECOND) break;
			usleep(((delay < MAX_SLEEP_TIME ? delay : MAX_SLEEP_TIME) - MasterClock::NANOS_PER_MILLISECOND) / MasterClock::NANOS_PER_MICROSECOND);
		}
		const QMidiEvent &e = midiEvents.at(currentEventIx);
		if (driver->stopProcessing || synthRoute->getState() != SynthRouteState_OPEN) break;
		switch (e.getType()) {
			case SHORT_MESSAGE:
				synthRoute->pushMIDIShortMessage(*session, e.getShortMessage(), currentNanos);
				break;
			case SYSEX:
				synthRoute->pushMIDISysex(*session, e.getSysexData(), e.getSysexLen(), currentNanos);
				break;
			case SET_TEMPO: {
				uint tempo = e.getShortMessage();
				midiTick = parser.getMidiTick(tempo);
				emit driver->tempoUpdated(MidiParser::MICROSECONDS_PER_MINUTE / tempo);
				break;
			}
			default:
				break;
		}
	}
	sendAllSoundOff(synthRoute, true, false);
	emit driver->playbackTimeChanged(0, 0);
	qDebug() << "SMFDriver: processor thread stopped";
	driver->deleteMidiSession(session);
	if (!driver->stopProcessing) emit driver->playbackFinished();
}

quint32 SMFProcessor::estimateRemainingTime(const QMidiEventList &midiEvents, int currentEventIx) {
	MasterClockNanos tick = midiTick;
	MasterClockNanos totalNanos = 0;
	for (int i = currentEventIx; i < midiEvents.count(); i++) {
		const QMidiEvent &e = midiEvents.at(i);
		totalNanos += e.getTimestamp() * tick;
		if (e.getType() == SET_TEMPO) tick = parser.getMidiTick(e.getShortMessage());
	}
	return quint32(totalNanos / MasterClock::NANOS_PER_SECOND);
}

void SMFProcessor::seek(SynthRoute *synthRoute, const QMidiEventList &midiEvents, int &currentEventIx, MasterClockNanos &currentEventNanos, const MasterClockNanos seekNanos) {
	while (!driver->stopProcessing && synthRoute->getState() == SynthRouteState_OPEN && currentEventNanos < seekNanos) {
		const QMidiEvent &e = midiEvents.at(currentEventIx);
		switch (e.getType()) {
			case SHORT_MESSAGE: {
				quint32 msg = e.getShortMessage();
				// Ignore NoteOn & NoteOff while seeking
				if ((msg & 0xE0) != 0x80) synthRoute->playMIDIShortMessageNow(msg);
				break;
			}
			case SYSEX:
				synthRoute->playMIDISysexNow(e.getSysexData(), e.getSysexLen());
				break;
			case SET_TEMPO: {
				uint tempo = e.getShortMessage();
				midiTick = parser.getMidiTick(tempo);
				emit driver->tempoUpdated(MidiParser::MICROSECONDS_PER_MINUTE / tempo);
				break;
			}
			default:
				break;
		}
		int nextEventIx = currentEventIx + 1;
		if (midiEvents.size() <= nextEventIx) break;
		currentEventIx = nextEventIx;
		currentEventNanos += midiEvents.at(currentEventIx).getTimestamp() * midiTick;
	}
}

SMFDriver::SMFDriver(Master *useMaster) : MidiDriver(useMaster), processor(this) {
	name = "Standard MIDI File Driver";
}

void SMFDriver::start() {
	static QString currentDir = NULL;
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(Master::getInstance()->getSettings()->value("Master/qFileDialogOptions", 0).toInt());
	QString fileName = QFileDialog::getOpenFileName(NULL, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*",
		NULL, qFileDialogOptions);
	currentDir = QDir(fileName).absolutePath();
	if (!fileName.isEmpty()) {
		stop();
		processor.start(fileName);
	}
}

void SMFDriver::start(QString fileName) {
	if (!fileName.isEmpty()) {
		stop();
		processor.start(fileName);
	}
}

void SMFDriver::stop() {
	stopProcessing = true;
	MidiDriver::waitForProcessingThread(processor, MAX_SLEEP_TIME);
}

void SMFDriver::pause(bool paused) {
	pauseProcessing = paused;
}

void SMFDriver::setBPM(quint32 newBPM) {
	bpmUpdate = newBPM;
}

void SMFDriver::setFastForwardingFactor(uint useFastForwardingFactor) {
	fastForwardingFactor = useFastForwardingFactor;
}

void SMFDriver::jump(int newPosition) {
	seekPosition = newPosition;
}

SMFDriver::~SMFDriver() {
	stop();
}
