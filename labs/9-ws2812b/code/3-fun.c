/*
 * simple test to use your buffered neopixel interface to push a cursor around
 * a light array.
 */
#include "rpi.h"
#include "WS2812B.h"
#include "neopixel.h"
#include "rpi-math.h"

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

typedef struct {
    int r;
    int g;
    int b;
} pixel_t;

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

pixel_t to_uint8(rgb in) {
    pixel_t out;
    out.r = in.r * 255;
    out.g = in.g * 255;
    out.b = in.b * 255;
    return out;
}

// the pin used to control the light strip.
enum { pix_pin = 21 };

void notmain(void) {
    caches_enable(); 
    gpio_set_output(pix_pin);

    // make sure when you implement the neopixel 
    // interface works and pushes a pixel around your light
    // array.
    unsigned npixels = 30;  // you'll have to figure this out.
    neo_t h = neopix_init(pix_pin, npixels);

    hsv v = {0, 1.0, 0.6};

    // does 10 increasingly faster loops.
    for(int j = 0; j < 360; j++) {
        output("loop %d\n", j);
        for(int i = 0; i < npixels; i++) {
            v.h = 360.0 * i / (npixels-1) + j;
            while (v.h > 360)
                v.h -= 360;
            rgb c = hsv2rgb(v);
            pixel_t p = to_uint8(c);
            // printk("hue %f rgb %f %f %f pixel %d %d %d\n", v.h, c.r, c.g, c.b, p.r, p.g, p.b);
            neopix_write(h, i, p.r, p.g, p.b);
            neopix_flush_keep(h);
            // delay_ms(10);
        }
        // delay_ms(100);
        // for(int i = npixels-1; i >= 0; i--) {
        //     neopix_write(h, i, 0, 0, 0);
        //     neopix_flush_keep(h);
        //     delay_ms(10);
        // }
        // neopix_clear(h);
        delay_ms(10);
    }
    neopix_clear(h);
    output("done!\n");
}
