#pragma once

#include "rpi.h"

void stepper_init();
void run_stepper(double power);
void step(int resolution, double speed);
