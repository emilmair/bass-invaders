#ifndef MES_MESGRAPHICS_H
#define MES_MESGRAPHICS_H

#include <math.h>
#include "stdint.h"
#include "stdlib.h"
#include <stdbool.h>

#define SURF_BPP 3
#define SURF_SIZE(X, Y) (((uint16_t)ceil((float)(SURF_BPP * ((X) * (Y))) / 8.0) % SURF_BPP) * 3 + ((SURF_BPP * ((X) * (Y))) / 8))
#define SURF_POSITION(SURF, X, Y) ((Y) * (SURF)->width + (X))
#define PIXEL_MASK ((1 << SURF_BPP) - 1)

struct Surface {
    uint8_t width;
    uint8_t height;
    void *data;
};

typedef struct Surface Surface;

static void swap(uint8_t *xp, uint8_t *yp) {
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

static void surf_resize(Surface *surf, uint8_t width, uint8_t height) {
    surf->width = width;
    surf->height = height;
    surf->data = realloc(surf->data, SURF_SIZE(width, height));
}

static void surf_destroy(Surface *surf) {
    free(surf->data);
    free(surf);
}

static void surf_set_pixel(Surface *surf, uint8_t x, uint8_t y, uint8_t color) {
    uint16_t pos = SURF_POSITION(surf, x, y);
    uint32_t *pixels = (uint32_t *) (surf->data + (pos / 8) * SURF_BPP);
    uint8_t shift = (7 - (pos % 8)) * SURF_BPP;
    uint32_t mask = PIXEL_MASK << shift;
    *pixels = (*pixels & ~mask) | ((color & PIXEL_MASK) << shift);
}

static uint8_t surface_get_pixel(Surface *surf, uint8_t x, uint8_t y) {
    uint16_t pos = SURF_POSITION(surf, x, y);
    uint32_t pixels = (*(uint32_t *) (surf->data + (pos / 8) * SURF_BPP));
    return (pixels >> ((7 - (pos % 8)) * SURF_BPP)) & PIXEL_MASK;
}

static void surf_draw_line(Surface *surf, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
    bool steep = false;
    if (abs(x0 - x1) < abs(y0 - y1)) {
        swap(&x0, &y0);
        swap(&x1, &y1);
        steep = true;
    }
    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int16_t derror2 = abs(dy) * 2;
    int16_t error2 = 0;
    uint8_t y = y0;
    for (uint8_t x = x0; x < x1; ++x) {
        if (steep) {
            surf_set_pixel(surf, y, x, color);
        } else {
            surf_set_pixel(surf, x, y, color);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

static void surf_fill(Surface *surf, uint8_t color) {
    if (color == 0b000) {memset(surf->data, 0x00, SURF_SIZE(surf->width, surf->height)); return;}
    else if (color == 0b111) {memset(surf->data, 0xFF, SURF_SIZE(surf->width, surf->height)); return;}
    for (int y = 0; y < surf->height; y++) {
        for (int x = 0; x < surf->width; x++) {
            surf_set_pixel(surf, x, y, color);
        }
    }
}

#endif
