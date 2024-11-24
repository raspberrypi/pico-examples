#ifndef CRGB_H
#define CRGB_H

#include <stdlib.h>

#include <cmath>

struct CRGB {
    uint8_t g, r, b, n = 0;

    CRGB() : r(0), g(0), b(0) {};
    CRGB(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {};

    CRGB(unsigned long rgb) {
        b = rgb & 0xffu;
        g = (rgb >> 8u) & 0xffu;
        r = (rgb >> 16u) & 0xffu;
    }

    uint8_t &operator[](int i) {
        switch (i) {
            case 0:
                return r;
                break;
            case 1:
                return g;
                break;
            case 2:
                return b;
            default:
                return n;
                break;
        };
    }

    CRGB operator*(double f) {
        CRGB res(static_cast<uint8_t>(ceil(r * f)), static_cast<uint8_t>(ceil(g * f)), static_cast<uint8_t>(ceil(b * f)));
        return res;
    }

    CRGB operator*(uint8_t f) {
        CRGB res(((uint16_t)r * (f + 1)) >> 8, ((uint16_t)g * (f + 1)) >> 8, ((uint16_t)b * (f + 1)) >> 8);
        return res;
    }

    CRGB operator/(double f) {
        return *this * (1.0 / f);
    }

    CRGB operator+(CRGB rgb2) {
        CRGB res(r + rgb2.r, g + rgb2.g, b + rgb2.b);
        return res;
    }

    static CRGB blend(CRGB rgb1, CRGB rgb2, double f) {
        return rgb1 * f + rgb2 * (1.0 - f);
    }

    CRGB operator=(uint32_t rgb) {
        b = rgb & 0xffu;
        g = (rgb >> 8u) & 0xffu;
        r = (rgb >> 16u) & 0xffu;
        return *this;
    }

    uint32_t toRGB() {
        return ((uint32_t)(r) << 16) |
               ((uint32_t)(g) << 8) |
               (uint32_t)(b);
    }

    uint32_t toGRB() {
        return ((uint32_t)(g) << 16) |
               ((uint32_t)(r) << 8) |
               (uint32_t)(b);
    }
};

#endif