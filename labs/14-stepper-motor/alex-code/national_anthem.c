/**
 * Here's the notes to the american national anthem
 *
 * This won't compile:
 * You'll have to tune your Pi and write some functions which can play
 * these notes.
 *
 * You can completely change the layout of this data if you'd like
 */

#include "stepper-int.h"
#include "rpi-math.h"

#define DIR 16
#define STEP 17
#define MS1 6
#define MS2 5
#define MS3 7

typedef enum {
    A,
    AS,
    B,
    C,
    CS,
    D,
    DS,
    E,
    F,
    FS,
    G,
    GS,
} stepper_tone_t;

typedef struct {
    stepper_tone_t tone;
    unsigned octave;
    unsigned duration;
} stepper_note_t;

const unsigned whole = 2500000;
const unsigned half =  whole/2;
const unsigned quarter = half/2;
const unsigned eighth = quarter/2;
const unsigned sixteenth = eighth/2;

stepper_note_t american_notes[] = {
        {C, 3, eighth + sixteenth},
        {A, 3, sixteenth},
        {F, 4, quarter}, // 1
        {A, 3, quarter},
        {C, 3, quarter},
        {F, 3, half}, // 2
        {A, 2, eighth + sixteenth},
        {G, 3, sixteenth},
        {F, 3, quarter}, // 3
        {A, 3, quarter},
        {B, 3, quarter},
        {C, 3, half}, // 4
        {C, 3, eighth},
        {C, 3, eighth},
        {A, 2, quarter + eighth}, // 5
        {G, 3, eighth},
        {F, 3, quarter},
        {E, 3, half}, // 6
        {D, 3, eighth},
        {E, 3, eighth},
        {F, 3, quarter},
        {F, 3, quarter},
        {C, 3, quarter},
        {A, 3, quarter},
        {F, 4, quarter},
        {C, 3, eighth + sixteenth},
        {A, 3, sixteenth},
        {F, 4, quarter},
        {A, 3, quarter},
        {C, 3, quarter},
        {F, 3, half},
        {A, 2, eighth + sixteenth},
        {G, 3, sixteenth},
        {F, 3, quarter},
        {A, 3, quarter},
        {B, 3, quarter},
        {C, 3, half},
        {C, 3, eighth},
        {C, 3, eighth},
        {A, 2, quarter + eighth},
        {G, 3, eighth},
        {F, 3, quarter},
        {E, 3, half},
        {D, 3, eighth},
        {E, 3, eighth},
        {F, 3, quarter},
        {F, 3, quarter},
        {C, 3, quarter},
        {A, 3, quarter},
        {F, 4, quarter},
        {A, 2, eighth},
        {A, 2, eighth},
        {A, 2, quarter},
        {AS,2, quarter},
        {C, 2, quarter},
        {C, 2, half},
        {AS,2, eighth},
        {A, 2, eighth},
        {G, 3, quarter},
        {A, 2, quarter},
        {AS,2, quarter},
        {AS,2, half},
        {AS,2, quarter}, // here
        {A, 2, quarter + eighth},
        {G, 3, eighth},
        {F, 3, quarter},
        {E, 3, half},
        {D, 3, eighth},
        {E, 3, eighth},
        {F, 3, quarter},
        {A, 3, quarter},
        {B, 3, quarter},
        {C, 3, half},
        {C, 3, quarter},
        {F, 3, quarter},
        {F, 3, quarter},
        {F, 3, eighth},
        {E, 3, eighth},
        {D, 3, quarter},
        {D, 3, quarter},
        {D, 3, quarter},
        {G, 3, eighth},
        {A, 2, eighth},
        {AS,2, eighth},
        {A, 2, eighth},
        {G, 3, eighth},
        {F, 3, eighth},
        {F, 3, quarter},
        {E, 3, quarter},
        {C, 3, eighth},
        {C, 3, eighth},
        {F, 3, quarter + eighth},
        {G, 3, eighth},
        {A, 2, eighth},
        {AS,2, eighth},
        {C, 2, half},
        {F, 3, eighth},
        {G, 3, eighth},
        {A, 2, quarter + eighth},
        {AS,2, eighth},
        {G, 3, quarter},
        {F, 3, half + quarter},
        {A, 3, 0}, // zero duration to mark end
    };

void play_note(stepper_t* stepper, stepper_note_t note) {
    int key = 1 + note.tone + 12 * note.octave;
    double freq = pow(2, (key - 49) / 12.0) * 440;
    unsigned start = timer_get_usec();
    while (timer_get_usec() - start < note.duration) {
        stepper_freq(stepper, freq);
    }
}

void play_song(stepper_t* stepper, stepper_note_t* song) {
    while (1) {
        stepper_note_t note = *song;
        if (note.duration == 0) {
            break;
        }
        play_note(stepper, note);
        song++;
    }
}


void notmain(){
    printk("Stepper: starting\n");
    stepper_int_t* stepper = stepper_init_with_int_with_microsteps(DIR, STEP, MS1, MS2, MS3, FULL_STEP);

    play_song(stepper->stepper, american_notes);
    printk("Done!\n");
}
