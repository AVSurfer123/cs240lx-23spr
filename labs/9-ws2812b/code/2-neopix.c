/*
 * simple test to use your buffered neopixel interface to push a cursor around
 * a light array.
 */
#include "rpi.h"
#include "WS2812B.h"
#include "neopixel.h"
#include "rpi-math.h"

void rgb_to_hsv(int r, int g, int b, int* h, int* s, int* v) {
    float rr, gg, bb;
    float max_val, min_val, delta;

    rr = r / 255.0;
    gg = g / 255.0;
    bb = b / 255.0;

    max_val = fmaxf(rr, fmaxf(gg, bb));
    min_val = fminf(rr, fminf(gg, bb));
    delta = max_val - min_val;

    if (delta == 0) {
        *h = 0;
    }
    else if (max_val == rr) {
        *h = (int) roundf((gg - bb) / delta);
    }
    else if (max_val == gg) {
        *h = (int) roundf((bb - rr) / delta + 2);
    }
    else {
        *h = (int) roundf((rr - gg) / delta + 4);
    }

    *h *= 60;
    if (*h < 0) {
        *h += 360;
    }

    if (max_val == 0) {
        *s = 0;
    }
    else {
        *s = (int) roundf(delta / max_val);
    }
    *s *= 100;

    *v = (int) roundf(max_val * 100);
}

// the pin used to control the light strip.
enum { pix_pin = 21 };

// crude routine to write a pixel at a given location.
void place_cursor(neo_t h, int i) {
    neopix_write(h,i-2,0x88,0,0);
    neopix_write(h,i-1,0,0x88,0);
    neopix_write(h,i,0,0,0x88);
    neopix_flush(h);
}

void notmain(void) {
    caches_enable(); 
    gpio_set_output(pix_pin);

    // make sure when you implement the neopixel 
    // interface works and pushes a pixel around your light
    // array.
    unsigned npixels = 30;  // you'll have to figure this out.
    neo_t h = neopix_init(pix_pin, npixels);

    // does 10 increasingly faster loops.
    for(int j = 0; j < 10; j++) {
        output("loop %d\n", j);
        for(int i = 2; i < npixels; i++) {
            place_cursor(h,i);
            delay_ms(10-j);
        }
    }
    neopix_clear(h);
    output("done!\n");
}
