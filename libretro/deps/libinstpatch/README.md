#	libInstPatch
###	Copyright (C) 1999-2021 Element Green and others
http://www.swamiproject.org

[![Build Status](https://dev.azure.com/tommbrt/tommbrt/_apis/build/status/swami.libinstpatch?branchName=master)](https://dev.azure.com/tommbrt/tommbrt/_build/latest?definitionId=1&branchName=master)

## 1. What is libInstPatch?

libInstPatch stands for lib-Instrument-Patch and is a library for processing digital sample based MIDI instrument "patch" files.
The types of files libInstPatch supports are used for creating instrument sounds for wavetable synthesis. libInstPatch provides
an object framework (based on GObject) to load patch files into, which can then be edited, converted, compressed and saved.

More information can be found on the libInstPatch Wiki pages on the Project Swami website.

http://www.swamiproject.org


## 2. License

See [Copying](https://github.com/swami/libinstpatch/blob/master/COPYING).

## 3. Requirements

Look at the INSTALL file for instructions on compiling and installing
libInstPatch and for more details on software requirements.

libInstPatch has the following requirements:
- glib/GObject >= 2.14
- libsndfile

glib/gobject homepage: http://www.gtk.org
libsndfile homepage: http://www.mega-nerd.com/libsndfile

libInstPatch can be built for Linux, Mac OSX and many other Unix like
operating systems.  It has also been known to run on Windows systems.


## 4. Features

* Native GObject C API
* Supports SoundFont 2
* SoundFont synthesis cache subsystem (IpatchSF2VoiceCache)
* Conversion of most raw sample width formats (8/16/24/32bit/float/double, etc)
* Sample format transform functions support up to 8 channels
* Sample cache pool for caching samples in RAM in different formats
* Sample edit lists for primitive sample editing operations
* Simple XML tree parsing/saving using the glib GNode data type
* Paste subsystem for easily performing copies of objects within and
  between different instrument files
* Instrument item conversion sub system
* Incomplete support for DLS 1/2 and GigaSampler


## 5. Trademark Acknowledgement
SoundFont is a registered trademark of E-mu Systems, Inc.
All other trademarks are property of their respective holders.
