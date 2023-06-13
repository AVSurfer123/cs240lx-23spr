#include "stepper-int.h"
#include "timer-interrupt.h"
#include "cycle-count.h"
#include "rpi-inline-asm.h"

// you can/should play around with this
#define STEPPER_INT_TIMER_INT_PERIOD 100

static int first_init = 1;

#define MAX_STEPPERS 16
static stepper_int_t * my_steppers[MAX_STEPPERS];
static unsigned num_steppers = 0;

void stepper_int_handler(unsigned pc) {
    // check and clear timer interrupt
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();

    for (int i = 0; i < num_steppers; i++) {
        if (Q_nelem(&my_steppers[i]->positions_Q) == 0) {
            my_steppers[i]->status = NOT_IN_JOB;
            continue;
        }
        stepper_position_t* top = my_steppers[i]->positions_Q.head;
        if (top->status == NOT_STARTED) {
            top->start_steps = stepper_int_get_position_in_steps(my_steppers[i]);
            top->start_time_usec = timer_get_usec();
            top->status = STARTED;
        }
        else if (top->status == STARTED) {
            unsigned num_steps = (timer_get_usec() - top->start_time_usec) / top->usec_between_steps;
            if (top->goal_steps > top->start_steps) {
                int diff = num_steps - (stepper_int_get_position_in_steps(my_steppers[i]) - top->start_steps);
                if (diff > 1) {
                    printk("diff is > 1: %d\n", diff);
                }
                for (int j = 0; j < diff; j++) {
                    stepper_step_forward(my_steppers[i]->stepper);
                }
            }
            else {
                int diff = num_steps - (top->start_steps - stepper_int_get_position_in_steps(my_steppers[i]));
                if (diff > 1) {
                    printk("diff is > 1: %d\n", diff);
                }
                for (int j = 0; j < diff; j++) {
                    stepper_step_backward(my_steppers[i]->stepper);
                }
            }
            if (num_steps == top->goal_steps - top->start_steps || num_steps == top->start_steps - top->goal_steps) {
                top->status = FINISHED;
                Q_pop(&my_steppers[i]->positions_Q);
            }
        }
    }
}

void interrupt_vector(unsigned pc){
    stepper_int_handler(pc);
}

stepper_int_t * stepper_init_with_int(unsigned dir, unsigned step){
    if(num_steppers == MAX_STEPPERS){
        return NULL;
    }
    // kmalloc_init();

    stepper_t* s = stepper_init(dir, step);

    stepper_int_t *stepper = kmalloc(sizeof *stepper);
    stepper->stepper = s;
    stepper->status = NOT_IN_JOB;
    my_steppers[num_steppers] = stepper;
    num_steppers++;

    //initialize interrupts; only do once, on the first init
    if(first_init){
        first_init = 0;
        int_init();
        cycle_cnt_init();
        timer_interrupt_init(STEPPER_INT_TIMER_INT_PERIOD);
        cpsr_int_enable();
    }

    return stepper;
}

stepper_int_t * stepper_init_with_int_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    if(num_steppers == MAX_STEPPERS){
        return NULL;
    }
    // kmalloc_init();

    stepper_t* s = stepper_init_with_microsteps(dir, step, MS1, MS2, MS3, microstep_mode);

    stepper_int_t *stepper = kmalloc(sizeof *stepper);
    stepper->stepper = s;
    stepper->status = NOT_IN_JOB;
    my_steppers[num_steppers] = stepper;
    num_steppers++;

    //initialize interrupts; only do once, on the first init
    if(first_init){
        first_init = 0;
        int_init();
        cycle_cnt_init();
        timer_interrupt_init(STEPPER_INT_TIMER_INT_PERIOD);
        cpsr_int_enable();
    }

    return stepper;
}

/* retuns the enqueued position. perhaps return the queue of positions instead? */
stepper_position_t * stepper_int_enqueue_pos(stepper_int_t * stepper, int goal_steps, unsigned usec_between_steps){
    stepper_position_t *new_pos = kmalloc(sizeof(stepper_position_t));
    new_pos->goal_steps = goal_steps;
    new_pos->usec_between_steps = usec_between_steps;
    new_pos->status = NOT_STARTED;
    Q_append(&stepper->positions_Q, new_pos);
    stepper->status = IN_JOB;
    return new_pos;
}

int stepper_int_get_position_in_steps(stepper_int_t * stepper){
    return stepper_get_position_in_steps(stepper->stepper);
}

int stepper_int_is_free(stepper_int_t * stepper){
    return stepper->status == NOT_IN_JOB;
}

int stepper_int_position_is_complete(stepper_position_t * pos){
    return pos->status == FINISHED;
}

