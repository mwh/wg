#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <time.h>
#include <sys/nearptr.h>

#include "grace.h"
#include "gc.h"

union REGS in,out;
__dpmi_regs regs;

typedef unsigned char byte;
byte *VGA = (byte*)0xA0000000L;

byte *rom_font_8x8;

byte video_mode;

byte text_fg;
byte text_bg;

static PendingStep *vga_mode_set(GraceObject *s, Env *e, GraceObject **args,
                             int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    double md = grace_number_val(args[0]);
    video_mode = (int)md;

    memset(&regs, 0, sizeof(regs));
    regs.h.ah = 00;
    regs.h.al = video_mode;
    __dpmi_int(0x10, &regs);

    if (video_mode == 0x13) {
        regs.x.ax = 0x1130;
        regs.h.bh = 0x03; // Request pointer to the ROM 8x8 font
        __dpmi_int(0x10, &regs);

        // Convert segmented to real flat physical address
        unsigned long font_phys_addr = (regs.x.es << 4) + regs.x.bp;
        rom_font_8x8 = (unsigned char *)(__djgpp_conventional_base + font_phys_addr);

        VGA = (unsigned char *)(__djgpp_conventional_base + 0xA0000);
        if (__djgpp_nearptr_enable() == 0) {
            printf("Error: Could not enable DJGPP near pointers!\n");
        }
    } else {
        __djgpp_nearptr_disable();
    }
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

static PendingStep *vga_textcolour(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    int col = (int) grace_number_val(args[0]);
    textcolor(col);
    text_fg = col;
    return cont_apply(k, grace_done);
}

static PendingStep *vga_textbg(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    int col = (int) grace_number_val(args[0]);
    textbackground(col);
    text_bg = col;
    return cont_apply(k, grace_done);
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
    if (ms < 0) ms = 0;
    delay(ms);
    return cont_apply(k, grace_done);
}

static PendingStep *vga_clock(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    GraceObject *val = grace_number_new(clock());
    return cont_apply(k, val);
}

static PendingStep *vga_timeofday(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    double r = tv.tv_usec;
    r /= 1000000;
    r += tv.tv_sec;
    GraceObject *val = grace_number_new(r);
    return cont_apply(k, val);
}

static PendingStep *vga_drawSprite(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    GraceBlob *blob = (GraceBlob *)args[0];
    int width = (int)grace_number_val(args[1]);
    int height = (int)grace_number_val(args[2]);
    int bx = (int)grace_number_val(args[3]);
    int by = (int)grace_number_val(args[4]);
    unsigned short offset;
    for (int y = 0; y < height; y++) {
        int cy = (by + y) * 320;
        for (int x = 0; x < width; x++) {
            unsigned char col = blob->value[y * height + x];
            offset = cy + bx + x;
            if (col != 255) {
	            _farpokeb(_dos_ds, 0xa0000 + offset, col);
            }
        }
    }
    return cont_apply(k, grace_done);
}

static PendingStep *vga_gc(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    gc_trigger();
    return cont_apply(k, grace_done);
}

static PendingStep *vga_gcliving(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    const GCStats *stats = gc_stats();
    return cont_apply(k, grace_number_new(stats->heap_size));
}

static PendingStep *vga_gc_assign(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    if (grace_bool_val(args[0])) {
        gc_unsuppress();
    } else {
        gc_suppress();
    }
    return cont_apply(k, grace_done);
}

void draw_char_with_bg(int x, int y, char c, unsigned char fg_color, unsigned char bg_color) {
    // Index into font table to find character (8 bit rows*8 bytes each character)
    unsigned char *glyph = &rom_font_8x8[(unsigned char)c * 8];

    for (int row = 0; row < 8; row++) {
        unsigned char bitmask = glyph[row];
        for (int col = 0; col < 8; col++) {
            int screen_x = x + col;
            int screen_y = y + row;
            if (screen_x >= 0 && screen_x < 320 && screen_y >= 0 && screen_y < 200) {
                int pixel_offset = screen_y * 320 + screen_x;
                // Foreground for 1, background for 0
                if (bitmask & (0x80 >> col)) {
                    VGA[pixel_offset] = fg_color;
                } else {
                    VGA[pixel_offset] = bg_color;
                }
            }
        }
    }
}

// Iterates through a string to draw text cleanly across pixel coordinates
void draw_string_with_bg(int x, int y, const char *str, unsigned char fg_color, unsigned char bg_color) {
    while (*str) {
        draw_char_with_bg(x, y, *str, fg_color, bg_color);
        x += 8;
        str++;
    }
}

static PendingStep *vga_puts(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    GraceObject *sObj = args[0];
    if (sObj->vt != &grace_string_vtable) {
        return grace_raise(e, "TypeError", "vga.puts given non-string argument");
    }
    const char *str = grace_string_val(sObj);
    if (video_mode == 0x13) {
        // Get initial cursor position
        regs.h.ah = 0x03;
        regs.h.bh = 0x00;
        __dpmi_int(0x10, &regs);
        int row = regs.h.dh;
        int col = regs.h.dl;
        draw_string_with_bg((col - 1) * 10, row * 8, str, text_fg, text_bg);
    } else {
        cputs(str);
    }
    return cont_apply(k, grace_done);
}

static PendingStep *vga_putsAt(GraceObject *s, Env *e,
        GraceObject **args, int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;(void)args;
    GraceObject *sObj = args[0];
    if (sObj->vt != &grace_string_vtable) {
        return grace_raise(e, "TypeError", "vga.putsAt given non-string argument");
    }
    const char *str = grace_string_val(sObj);
    int x = grace_number_val(args[1]);
    int y = grace_number_val(args[2]);
    if (video_mode == 0x13) {
        draw_string_with_bg(x, y, str, text_fg, text_bg);
    } else {
        gotoxy(x, y);
        cputs(str);
    }
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
    user_add_method(p, "textcolour(1)", vga_textcolour, NULL);
    user_add_method(p, "textbg(1)", vga_textbg, NULL);
    user_add_method(p, "puts(1)", vga_puts, NULL);
    user_add_method(p, "puts(1)at(2)", vga_putsAt, NULL);
    user_add_method(p, "mouseX(0)", vga_mouseX, NULL);
    user_add_method(p, "mouseY(0)", vga_mouseY, NULL);
    user_add_method(p, "drawSprite(3)atX(1)Y(1)", vga_drawSprite, NULL);
    user_add_method(p, "mouseButton(0)", vga_mouseButton, NULL);
    user_add_method(p, "kbHit(0)", vga_kbhit, NULL);
    user_add_method(p, "goToXY(2)", vga_goToXY, NULL);
    user_add_method(p, "delay(1)", vga_delay, NULL);
    user_add_method(p, "clock(0)", vga_clock, NULL);
    user_add_method(p, "timeofday(0)", vga_timeofday, NULL);
    user_add_method(p, "gc(0)", vga_gc, NULL);
    user_add_method(p, "gcliving(0)", vga_gcliving, NULL);
    user_add_method(p, "gc:=(1)", vga_gc_assign, NULL);
    return p;
}

// vim: set expandtab ts=4 sw=4:
