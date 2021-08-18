/*
 * Copyleft:
 * This file was made to be used with DOSBOX, and so
 * is under GNU general public license
 */

#ifndef __DOSBOX_PINHACK_H
#define __DOSBOX_PINHACK_H
#define PINHACKVERSION 3

extern struct scrollhack {
    bool enabled;
    bool trigger;
    struct {
        int min, max;
    } triggerwidth, triggerheight;
    struct {
        int height, width;
    } expand;
} pinhack;

#endif /* __DOSBOX_PINHACK_H */
