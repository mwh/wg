#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include "grace.h"
#include "gc.h"

union REGS in,out;
__dpmi_regs regs;

typedef unsigned char byte;
byte *VGA = (byte*)0xA0000000L;

static PendingStep *vga_mode_set(GraceObject *s, Env *e, GraceObject **args,
                             int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    double md = grace_number_val(args[0]);
    memset(&regs, 0, sizeof(regs));
    regs.h.ah = 00;
    regs.h.al = (int)md;
    __dpmi_int(0x10, &regs);
    return cont_apply(k, grace_true);
}

static PendingStep *vga_setColourAtXY_fn(GraceObject *s, Env *e,
        GraceObject **args,
        int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    unsigned short offset;
    double cd = grace_number_val(args[0]);
    double xd = grace_number_val(args[1]);
    double yd = grace_number_val(args[2]);

    int colour = ((int)cd) % 256;
    int x = (int)xd;
    int y = (int)yd;
    //VGA[y * 320 + x] = colour;
    offset = y * 320 + x;
	_farpokeb(_dos_ds, 0xa0000 + offset, colour);
    return cont_apply(k, grace_true);
}

static PendingStep *vga_setOffset_to(GraceObject *s, Env *e,
        GraceObject **args,
        int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    unsigned short offset;
    double os = grace_number_val(args[0]);
    double cd = grace_number_val(args[1]);

    int colour = ((int)cd) % 256;
    offset = (unsigned short)os;
	_farpokeb(_dos_ds, 0xa0000 + offset, colour);
    return cont_apply(k, grace_true);
}

static PendingStep *vga_goToXY(GraceObject *s, Env *e,
        GraceObject **args,
        int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    double xd = grace_number_val(args[0]);
    double yd = grace_number_val(args[1]);

    unsigned short x = (unsigned short)xd;
    unsigned short y = (unsigned short)yd;
    gotoxy(x, y);
    return cont_apply(k, grace_done);
}

static PendingStep *vga_rectFrom_to_colour(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    unsigned short x1, y1, x2, y2, colour;
    x1 = (unsigned short)grace_number_val(args[0]);
    y1 = (unsigned short)grace_number_val(args[1]);
    x2 = (unsigned short)grace_number_val(args[2]);
    y2 = (unsigned short)grace_number_val(args[3]);
    colour = (unsigned short)grace_number_val(args[4]);
    for (int x = x1; x < x2; x++) {
        for (int y = y1; y < y2; y++) {
            unsigned short offset = y * 320 + x;
            _farpokeb(_dos_ds, 0xa0000 + offset, colour);
        }
    }
    return cont_apply(k, grace_true);
}

static PendingStep *vga_getch(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    char *str = malloc(2);
    unsigned char c = getch();
    str[0] = c;
    str[1] = 0;
    GraceObject *gstr = grace_string_take(str);
    return cont_apply(k, gstr);
}

static PendingStep *vga_kbhit(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    if (kbhit())
        return cont_apply(k, grace_true);
    return cont_apply(k, grace_false);
}

static PendingStep *vga_mouseEnabled_assign(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    int on = grace_bool_val(args[0]);
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = on;
    __dpmi_int(0x33, &regs);
    if (on) {
        regs.x.ax = 4;
        regs.x.cx = 639;
        regs.x.dx = 199;
        __dpmi_int(0x33, &regs);
    }
    return cont_apply(k, grace_done);
}

static PendingStep *vga_mouseVisible_assign(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    int on = grace_bool_val(args[0]);
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = on ? 1 : 2;
    __dpmi_int(0x33, &regs);
    return cont_apply(k, grace_done);
}

static PendingStep *vga_mouseX(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 3;
    __dpmi_int(0x33, &regs);
    return cont_apply(k, grace_number_new(regs.x.cx));
}

static PendingStep *vga_mouseY(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 3;
    __dpmi_int(0x33, &regs);
    return cont_apply(k, grace_number_new(regs.x.dx));
}

static PendingStep *vga_mouseButton(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 3;
    __dpmi_int(0x33, &regs);
    return cont_apply(k, grace_number_new(regs.x.bx));
}

static PendingStep *vga_delay(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    int ms = (int)grace_number_val(args[0]);
    delay(ms);
    return cont_apply(k, grace_done);
}


GraceObject *make_vga(void) {
    GraceObject *p = grace_user_new(NULL);
    user_add_method(p, "mode:=(1)", vga_mode_set, NULL);
    user_add_method(p, "mouseEnabled:=(1)", vga_mouseEnabled_assign, NULL);
    user_add_method(p, "mouseVisible:=(1)", vga_mouseVisible_assign, NULL);
    user_add_method(p, "setColour(1)atX(1)Y(1)", vga_setColourAtXY_fn, NULL);
    user_add_method(p, "setOffset(1)to(1)", vga_setOffset_to, NULL);
    user_add_method(p, "at(1)put(1)", vga_setOffset_to, NULL);
    user_add_method(p, "rectFrom(2)to(2)colour(1)", vga_rectFrom_to_colour, NULL);
    user_add_method(p, "getch(0)", vga_getch, NULL);
    user_add_method(p, "mouseX(0)", vga_mouseX, NULL);
    user_add_method(p, "mouseY(0)", vga_mouseY, NULL);
    user_add_method(p, "mouseButton(0)", vga_mouseButton, NULL);
    user_add_method(p, "kbHit(0)", vga_kbhit, NULL);
    user_add_method(p, "goToXY(2)", vga_goToXY, NULL);
    user_add_method(p, "delay(1)", vga_delay, NULL);

    return p;
}

// vim: set expandtab ts=4 sw=4:
