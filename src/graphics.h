#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define SURF_BPP 3
#define SURF_SIZE(X, Y) (((uint16_t)ceil((float)(SURF_BPP * ((X) * (Y))) / 8.0) % SURF_BPP) * 3 + ((SURF_BPP * ((X) * (Y))) / 8))
#define SURF_POSITION(SURF, X, Y) ((Y) * (SURF)->width + (X))
#define PIXEL_MASK ((1 << SURF_BPP) - 1)

// rgb to gpio config
#define COLOR_TO_GPIO(red, green, blue) (uint16_t)                                         \
            (((red & 0b100) >> 1) | ((red & 0b010) << 9) | ((red & 0b001) << 11) |         \
            ((green & 0b100) << 10) | ((green & 0b010) << 12) | ((green & 0b001) << 14) |  \
            ((blue & 0b100) << 5) | ((blue & 0b010) << 5) | ((blue & 0b001) << 5))

// gpio config to rgb
#define RED_FROM_GPIO(color) (((color << 1) & 0b100) | ((color >> 9) & 0b010 ) | ((color >> 11) & 0b001))
#define GREEN_FROM_GPIO(color) (((color >> 10) & 0b100) | ((color >> 12) & 0b010) | ((color >> 14) & 0b001))
#define BLUE_FROM_GPIO(color) (((color >> 5) & 0b100) | ((color >> 7) & 0b010) | ((color >> 9) & 0b001))

typedef void (*func)();

typedef union {
    struct  {
        uint8_t width, height;
        void *data;
    };
    struct  {
        uint8_t w, h;
        void *d;
    };
} Surface;

static void swap(uint8_t* xp, uint8_t* yp) {
    uint8_t temp = *xp;
    *xp = *yp;
    *yp = temp;
}

static Surface surf_create(uint8_t width, uint8_t height) {
    return (Surface) {
        width, height, malloc(SURF_SIZE(width, height))
    };
}

static Surface surf_create_from_memory(uint8_t width, uint8_t height, void* data) {
    return (Surface) {
            width, height, data
    };
}

static void surf_resize(Surface* surf, uint8_t width, uint8_t height) {
    surf->width = width;
    surf->height = height;
    surf->data = realloc(surf->data, SURF_SIZE(width, height));
}

static void surf_destroy(Surface *surf) {
    free(surf->data);
}

static void surf_set_pixel(Surface* surf, uint8_t x, uint8_t y, uint8_t color) {
    uint16_t pos = SURF_POSITION(surf, x, y);
    uint32_t *pixels = (uint32_t *) (surf->data + (pos / 8) * SURF_BPP);
    uint8_t shift = (7 - (pos % 8)) * SURF_BPP;
    uint32_t mask = PIXEL_MASK << shift;
    *pixels = (*pixels & ~mask) | ((color & PIXEL_MASK) << shift);
}

static uint8_t surf_get_pixel(Surface* surf, uint8_t x, uint8_t y) {
    uint16_t pos = SURF_POSITION(surf, x, y);
    uint32_t pixels = (*(uint32_t *) (surf->data + (pos / 8) * SURF_BPP));
    return (pixels >> ((7 - (pos % 8)) * SURF_BPP)) & PIXEL_MASK;
}


/**
 * Bresenham curve rasterizing algorithms implemented by Alois Zingl
 * https://github.com/zingl/Bresenham, MIT License
*/

static void surf_draw_line(Surface* surf, int x0, int y0, int x1, int y1, uint8_t color) {
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    for (;;) {
        surf_set_pixel(surf, x0, y0, color);
        e2 = 2*err;
        if (e2 >= dy) {
            if (x0 == x1) break;
            err += dy; x0 += sx;
        }
        if (e2 <= dx) {
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }
}

static void surf_draw_circle(Surface* surf, int xm, int ym, int r, uint8_t color) {
    int x = -r, y = 0, err = 2-2*r;
    do {
        surf_set_pixel(surf, xm-x, ym+y, color);
        surf_set_pixel(surf, xm-y, ym-x, color);
        surf_set_pixel(surf, xm+x, ym-y, color);
        surf_set_pixel(surf, xm+y, ym+x, color);
        r = err;
        if (r <= y) err += ++y*2+1;
        if (r > x || err > y) err += ++x*2+1;
    } while (x < 0);
}

static void surf_draw_ellipse(Surface* surf, int xm, int ym, int a, int b, uint8_t color) {
    int x = -a, y = 0;
    long e2 = (long)b*b, err = (long)x*(2*e2+x)+e2;
    do {
        surf_set_pixel(surf, xm-x, ym+y, color);
        surf_set_pixel(surf, xm+x, ym+y, color);
        surf_set_pixel(surf, xm+x, ym-y, color);
        surf_set_pixel(surf, xm-x, ym-y, color);
        e2 = 2*err;
        if (e2 >= (x*2+1)*(long)b*b) err += (++x*2+1)*(long)b*b;
        if (e2 <= (y*2+1)*(long)a*a) err += (++y*2+1)*(long)a*a;
    } while (x <= 0);
    while (y++ < b) {
        surf_set_pixel(surf, xm, ym+y, color);
        surf_set_pixel(surf, xm, ym-y, color);
    }
}

/**
 * Thanks Alois!
*/


/// fills surface with any given color.
static void surf_fill(Surface* surf, uint8_t color) {
    if (color == 0b000) {memset(surf->data, 0x00, SURF_SIZE(surf->width, surf->height)); return;}
    else if (color == 0b111) {memset(surf->data, 0xFF, SURF_SIZE(surf->width, surf->height)); return;}
    for (int y = 0; y < surf->height; y++) {
        for (int x = 0; x < surf->width; x++) {
            surf_set_pixel(surf, x, y, color);
        }
    }
}

/// draws source_surf onto destination_surf
static void surf_draw_surf_fast(Surface* destination_surf, Surface* source_surf, uint8_t x, uint8_t y) {
    for (int i = 0; i < source_surf->height; i++) {
        for (int j = 0; j < source_surf->width; j++) {
            surf_set_pixel(destination_surf, j+x, i+y, surf_get_pixel(source_surf, j, i));
        }
    }
}

/// draws source_surf onto destination_surf, supports partially out of bounds surfaces and signed coordinates
static void surf_draw_surf(Surface* destination_surf, Surface* source_surf, int16_t x, int16_t y) {
    for (int i = 0; i < source_surf->height; i++) {
        for (int j = 0; j < source_surf->width; j++) {
            if (j+x > destination_surf->w || i+y > destination_surf->h) continue;
            surf_set_pixel(destination_surf, j+x, i+y, surf_get_pixel(source_surf, j, i));
        }
    }
}

/// draws every source_surf pixel different from alpha onto destination_surf
static void surf_draw_surf_alpha_fast(Surface* destination_surf, Surface* source_surf, uint8_t x, uint8_t y, uint8_t alpha_color) {

}

/// draws every source_surf pixel different from alpha onto destination_surf, supports partially out of bounds surfaces and signed coordinates
static void surf_draw_surf_alpha(Surface* destination_surf, Surface* source_surf, uint8_t x, uint8_t y, uint8_t alpha_color) {

}

static void surf_draw_rectangle(Surface* surf, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
    uint8_t x0, y0, x1, y1;
    x0 = x;
    y0 = y;
    x1 = x+width;
    y1 = y+height;
    surf_draw_line(surf, x0, y0, x1, y0, color);
    surf_draw_line(surf, x1, y0, x1, y1, color);
    surf_draw_line(surf, x1, y1, x0, y1, color);
    surf_draw_line(surf, x0, y1, x0, y0, color);
}

static void surf_draw_filled_rectangle(Surface* surf, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            surf_set_pixel(surf, j+x, i+y, color);
        }
    }
}

static void surf_draw_filled_rectangle_fast(Surface* surf, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            surf_set_pixel(surf, j+x, i+y, color);
        }
    }
}

#endif //GRAPHICS_H
