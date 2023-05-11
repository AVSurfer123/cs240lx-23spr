#include "rpi.h"
#include "mpu-6050.h"
#include "rpi-math.h"
#include "quaternion.h"
#include "stepper.h"

extern quaternion_t q_est;

imu_xyz_t calc_bias(void* sensor, int use_gyro) {
    imu_xyz_t bias = {0};
    #define N_SAMPLES 1000
    imu_xyz_t samples[N_SAMPLES];

    for (int i = 0; i < N_SAMPLES; i++) {
        if (use_gyro) {
            gyro_t* g = (gyro_t*) sensor;
            samples[i] = gyro_scale(g, gyro_rd(g));
        }
        else {
            accel_t* a = (accel_t*) sensor;
            samples[i] = accel_scale(a, accel_rd(a));
        }
        bias.x += samples[i].x;
        bias.y += samples[i].y;
        bias.z += samples[i].z;
        delay_us(500);
    }
    bias.x /= N_SAMPLES;
    bias.y /= N_SAMPLES;
    bias.z /= N_SAMPLES;

    imu_xyz_t var = {0};
    for (int i = 0; i < N_SAMPLES; i++) {
        var.x += (samples[i].x - bias.x) * (samples[i].x - bias.x);
        var.y += (samples[i].y - bias.y) * (samples[i].y - bias.y);
        var.z += (samples[i].z - bias.z) * (samples[i].z - bias.z);
    }
    var.x = sqrt(var.x / N_SAMPLES);
    var.y = sqrt(var.y / N_SAMPLES);
    var.z = sqrt(var.z / N_SAMPLES);
    xyz_print("Standard deviation of measurements:", var);

    if (!use_gyro) {
        bias.z -= 1000; // subtract 1g from z-axis for gravity
    }

    return bias;
}

angles_t get_data(void* sensor, int use_gyro, imu_xyz_t bias) {
    imu_xyz_t reading;
    if (use_gyro) {
        gyro_t* g = (gyro_t*) sensor;
        reading = gyro_scale(g, gyro_rd(g));
    }
    else {
        accel_t* a = (accel_t*) sensor;
        reading = accel_scale(a, accel_rd(a));
    }
    reading.x -= bias.x;
    reading.y -= bias.y;
    reading.z -= bias.z;
    angles_t angle;
    double scale;
    if (use_gyro) {
        scale = M_PI / 180.0;
    }
    else {
        scale = 1 / 1000.0;
    }
    angle.x = reading.x * scale;
    angle.y = reading.y * scale;
    angle.z = reading.z * scale;
    return angle;
}

angles_t calc_tilt(angles_t accel) {
    angles_t tilt;
    tilt.x = atan2(accel.x, sqrt(accel.y * accel.y + accel.z * accel.z)) * 180 / M_PI;
    tilt.y = atan2(accel.y, sqrt(accel.x * accel.x + accel.z * accel.z)) * 180 / M_PI;
    tilt.z = atan2(sqrt(accel.x * accel.x + accel.y * accel.y), accel.z) * 180 / M_PI;
    return tilt;
}


void notmain() {
    stepper_init();
    printk("Initializing IMU...\n");
    accel_t a;
    gyro_t g;
    imu_init(&a, &g);

    printk("Calculating IMU bias...\n");
    imu_xyz_t accel_bias = calc_bias(&a, 0);
    imu_xyz_t gyro_bias = calc_bias(&g, 1);
    xyz_print("Accel bias:", accel_bias);
    xyz_print("Gyro bias:", gyro_bias);

    // for(int i = 0; i < 100; i++) {
    //     imu_xyz_t accel_raw = accel_rd(&a);
    //     imu_xyz_t gyro_raw = gyro_rd(&g);
    //     imu_xyz_t accel_clean = get_data(&a, 0, accel_bias);
    //     imu_xyz_t gyro_clean = get_data(&g, 1, gyro_bias);
    //     output("reading %d\n", i);
    //     xyz_print("\traw accel", accel_raw);
    //     xyz_print("\tscaled (1000=1g)", accel_scale(&a, accel_raw));
    //     xyz_print("\tcleaned", accel_clean);
    //     xyz_print("\traw gyro", gyro_raw);
    //     xyz_print("\tscaled (milli dps)", gyro_scale(&g, gyro_raw));
    //     xyz_print("\tcleaned", gyro_clean);
    //     angles_print("\ttilt angles", calc_tilt(accel_clean));
    //     delay_ms(500);
    // }

    double kp = -.01; // -.04
    double ki = -.0001; // .0005
    double total_error = 0;

    double last_time = timer_get_usec() / 1000000.0;
    for (int i = 0; i < 20000; i++) {
        // uint32_t start = timer_get_usec();
        angles_t accel_clean = get_data(&a, 0, accel_bias);
        angles_t gyro_clean = get_data(&g, 1, gyro_bias);
        
        last_time = imu_filter(accel_clean, gyro_clean, last_time);
        angles_t euler = eulerAngles(q_est);
        // angles_t tilt = calc_tilt(accel_clean);
        // printk("Pitch: %f\n", euler.y);
        // angles_print("Euler", euler);
        // angles_print("Tilt", tilt);

        // Threshold +/- 2 degrees
        float val = euler.y;
        // float val = tilt.x;
        if (fabs(val) > 2) {
            if (fabs(total_error) > 500) {
                total_error = 0;
            }
            total_error += val * .001;
            double power = kp * val + ki * total_error;
            power = fmin(1, fmax(-1, power));
            // printk("Power %f\n", power);
            run_stepper(power);
        }
        else {
            total_error = 0;
        }
        // uint32_t end = timer_get_usec();
        // printk("Iter time: %d\n", end - start);
    }
}
