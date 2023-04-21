#include "rpi.h"

void notmain(void) {
    // print out the string in the header.

    // figure out where it points to!
    const char *header_string = (char*) 0x8004;

    assert(header_string);
    printk("<%s>\n", header_string);
    printk("success!\n");
    clean_reboot();
}
