//main.c
#include "uart-interrupt.h"
#include "lcd.h"
#include "cyBot_Scan.h"
#include <stdio.h>
#include "open_interface.h"
#include "movement.h"
#include <math.h>
#include <stdbool.h>

// Constants for detection and filtering
#define MIN_OBJECT_WIDTH 4
#define MAX_OBJECT_WIDTH 60
#define MIN_DISTANCE 2.0
#define MAX_DISTANCE 110.0
#define DISTANCE_THRESHOLD 100.0
#define NUM_SAMPLES 3
#define MIN_VALID_READINGS 2
#define DISTANCE_CHANGE_THRESHOLD 2.0
#define CYBOT_LENGTH 20.0
#define IR_THRESHOLD 500 // Example threshold value
extern volatile int command_flag_manual;

// Constants for manual mode
#define FORWARD_SPEED 150
#define BACKWARD_SPEED -150
#define TURN_ANGLE 15

int servo_position = 0;  // Global variable declaration
// Don't redeclare these variables since they're already in uart-interrupt.h
// extern volatile int command_flag_go;
// extern volatile int command_flag_stop;

typedef struct {
    int object_number;
    int start_angle;
    int end_angle;
    float distance;
    float radial_width;
} ObjectInfo;

typedef struct {
    float sound_dist_arr[91]; // For each angle
    float IR_raw_val_arr[91];
    float IR_val_arr[91];
    int obj_num[8];
    int obj_fir_angle[8];
    int obj_las_angle[8];
    int obj_mid_angle[8];
    float obj_dist[8];
    float obj_IR_dist[8];
    float obj_radial_len[8];
    float obj_width[8];
} full_scan_data_t;

void string_Printf(const char* str) {
    while (*str != '\0') {
        uart_sendChar(*str);
        str++;
    }
}

float get_filtered_distance(int angle) {
    float total_distance = 0;
    int valid_readings = 0;
    cyBOT_Scan_t sensor_data;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++) {
        cyBOT_Scan(angle, &sensor_data);
        if (sensor_data.sound_dist >= MIN_DISTANCE && sensor_data.sound_dist <= MAX_DISTANCE) {
            total_distance += sensor_data.sound_dist;
            valid_readings++;
        }
    }

    return (valid_readings >= MIN_VALID_READINGS) ? (total_distance / valid_readings) : 9999;
}

void drive_to_object(oi_t *sensor, int target_angle, float target_distance) {
    char msg[100];
    double distance_mm = (target_distance - 30) * 10; // Convert cm to mm
    double sum = 0;

    // Turn to face the target
    if (target_angle > 90) {
        turn_left(sensor, target_angle - 107);
    } else {
        turn_right(sensor, 77 - target_angle);
    }
    timer_waitMillis(300);  // Allow time for turning to settle

    // Drive toward the object with obstacle avoidance
    while (sum < distance_mm) {
        oi_update(sensor);

        // Check for bumpers
        if (sensor->bumpLeft || sensor->bumpRight) {
            oi_setWheels(0, 0); // Stop immediately

            if (sensor->bumpLeft) {
                string_Printf("\r\nLeft bump detected! Avoiding obstacle...\r\n");
                move_backward(sensor, 150);  // Back up 15cm
                turn_right(sensor, 60);      // Turn right by 45 degrees
                move_forward(sensor, 300);   // Move forward a bit
                turn_left(sensor, 80);       // Realign by turning left
            } else if (sensor->bumpRight) {
                string_Printf("\r\nRight bump detected! Avoiding obstacle...\r\n");
                move_backward(sensor, 150);  // Back up 15cm
                turn_left(sensor, 50);       // Turn left by 45 degrees
                move_forward(sensor, 300);   // Move forward a bit
                turn_right(sensor, 90);      // Realign by turning right
            }

            timer_waitMillis(500); // Allow time for movements to settle

            // Rescan after avoiding the obstacle
            string_Printf("\r\nRescanning after obstacle avoidance...\r\n");
            scan_and_detect_objects(sensor, 0, 180, 5);
            return; // Exit current movement and let rescan handle further navigation
        }

        // If no bump detected, move forward in small increments
        oi_setWheels(150, 150);   // Move forward at a moderate speed
        timer_waitMillis(50);     // Small movement interval
        oi_update(sensor);
        sum += sensor->distance;  // Update distance traveled

        // Emergency stop condition (optional)
        if (command_flag_stop == 1) {
            oi_setWheels(0, 0);
            string_Printf("\r\nEmergency stop triggered!\r\n");
            return;
        }
    }

    // Stop at the target distance
    oi_setWheels(0, 0);
    sprintf(msg, "\r\nStopped %.1f cm from object\r\n", target_distance - (sum / 10.0));
    string_Printf(msg);
}

void scan_and_detect_objects(oi_t *sensor, int start_angle, int end_angle, int increment) {
    ObjectInfo objects[10];
    int object_count = 0;
    int in_object = 0;
    int start = 0;
    float min_distance = 9999;
    char msg[100];
    float prev_distance = 9999;
    float distance;
    float width;
    int angle;
    int i;

    string_Printf("\r\nStarting scan...\r\n");

    full_scan_data_t *full_scan_data = full_scan_alloc();

    for (angle = start_angle; angle <= end_angle; angle += increment) {
        // Take multiple IR readings and average them
        if (command_flag_stop == 1) { // Check for stop command
                    command_flag_stop = 0;
                    string_Printf("\r\nScan stopped.\r\n");
                    break;
        }
        int scan_IR_ave = 0;
        cyBOT_Scan_t sensor_data;
        cyBOT_Scan(angle, &sensor_data);
        scan_IR_ave += sensor_data.IR_raw_val;
        cyBOT_Scan(angle, &sensor_data);
        scan_IR_ave += sensor_data.IR_raw_val;
        scan_IR_ave /= 2.0;

        full_scan_update(&sensor_data, full_scan_data, angle, scan_IR_ave);
        scan_send(&sensor_data, angle);

        distance = get_filtered_distance(angle);
        sprintf(msg, "Angle %d: Distance = %.1f cm\r\n", angle, distance);
        string_Printf(msg);

        if (distance < DISTANCE_THRESHOLD && distance != 9999.0) {
            if (!in_object || (in_object && fabs(distance - prev_distance) <= DISTANCE_CHANGE_THRESHOLD)) {
                if (!in_object) {
                    in_object = 1;
                    start = angle;
                }
                if (distance < min_distance) {
                    min_distance = distance;
                }
            }
        } else {
            if (in_object) {
                width = angle - start;
                if (width >= MIN_OBJECT_WIDTH && width <= MAX_OBJECT_WIDTH) {
                    objects[object_count].object_number = object_count + 1;
                    objects[object_count].start_angle = start;
                    objects[object_count].end_angle = angle - increment;
                    objects[object_count].distance = min_distance;
                    objects[object_count].radial_width = width;
                    object_count++;
                }
                in_object = 0;
                min_distance = 9999;
            }
        }
        prev_distance = distance;
    }

    if (in_object) {
        width = end_angle - start;
        if (width >= MIN_OBJECT_WIDTH && width <= MAX_OBJECT_WIDTH) {
            objects[object_count].object_number = object_count + 1;
            objects[object_count].start_angle = start;
            objects[object_count].end_angle = end_angle;
            objects[object_count].distance = min_distance;
            objects[object_count].radial_width = width;
            object_count++;
        }
    }

    string_Printf("\r\nDetected Objects:\r\n");
    for (i = 0; i < object_count; i++) {
        // Use actual distance measurements for objects
        float actual_distance = get_filtered_distance((objects[i].start_angle + objects[i].end_angle) / 2);
        if (actual_distance != 9999.0) {
            objects[i].distance = actual_distance;
        }
        sprintf(msg, "Object %d: Start=%d, End=%d, Dist=%.1f cm, Width=%.1f deg\r\n",
                objects[i].object_number,
                objects[i].start_angle,
                objects[i].end_angle,
                objects[i].distance,
                objects[i].radial_width);
        string_Printf(msg);
    }

    if (object_count > 0) {
        int smallest_index = 0;
        float smallest_width = objects[0].radial_width;

        for (i = 1; i < object_count; i++) {
            if (objects[i].radial_width < smallest_width) {
                smallest_width = objects[i].radial_width;
                smallest_index = i;
            }
        }

        int target_angle = (objects[smallest_index].start_angle +
                          objects[smallest_index].end_angle) / 2;
        float target_distance = objects[smallest_index].distance;

        // Point sensor at target before moving
        cyBOT_Scan_t sensor_data;
        cyBOT_Scan(target_angle, &sensor_data);
        timer_waitMillis(500);  // Wait for servo to settle

        // Add this line to ensure servo stays at position
        servo_position = target_angle;
        cyBOT_Scan(target_angle, &sensor_data);
        timer_waitMillis(100);

        sprintf(msg, "\r\nPointing to smallest object (#%d) at angle %d\r\n",
                objects[smallest_index].object_number, target_angle);
        string_Printf(msg);

        drive_to_object(sensor, target_angle, target_distance);
    } else {
        string_Printf("\r\nNo objects detected\r\n");
    }

    full_scan_data_free(full_scan_data);
}

void full_scan_update(cyBOT_Scan_t *scan_data, full_scan_data_t *full_scan_data, int i, int scan_IR_ave) {
    full_scan_data->sound_dist_arr[i/2] = scan_data->sound_dist;
    full_scan_data->IR_raw_val_arr[i/2] = scan_IR_ave;
    full_scan_data->IR_val_arr[i/2]  = (420.160 * exp(-0.00244594 * (scan_IR_ave)) + 2.056);
}

void scan_send(cyBOT_Scan_t *scan_data, int deg) {
    char msg[100];
    sprintf(msg, "Degrees: %d\tDistance: %.3f cm\tIR raw: %d\r\n", deg, scan_data->sound_dist, scan_data->IR_raw_val);
    string_Printf(msg);
}


void full_scan_data_free(full_scan_data_t *self) {
    free(self);
}

full_scan_data_t *full_scan_alloc() {
    return calloc(1, sizeof(full_scan_data_t));
}

void sendBytes(char* message, double data){
    char final_mes[100];

    if (data != -10000) {
        sprintf(final_mes, "%s %.3f", message, data);
    } else {
        strcpy(final_mes, message);
    }
    string_Printf(final_mes);
}

void sendBytesInt(char* message, int data){
    char final_mes[100];

    sprintf(final_mes, "%s %d", message, data);
    string_Printf(final_mes);
}

// Function to process manual mode commands
void process_manual_command(oi_t *sensor, char command) {
    char msg[100];

    switch (command) {
        case 'w': // Forward
            string_Printf("\r\nMoving forward\r\n");
            oi_setWheels(FORWARD_SPEED, FORWARD_SPEED);
            timer_waitMillis(100);
            oi_setWheels(0, 0);
            break;

        case 's': // Backward
            string_Printf("\r\nMoving backward\r\n");
            oi_setWheels(BACKWARD_SPEED, BACKWARD_SPEED);
            timer_waitMillis(100);
            oi_setWheels(0, 0);
            break;

        case 'a': // Left
            string_Printf("\r\nTurning left\r\n");
            turn_left(sensor, TURN_ANGLE);
            break;

        case 'd': // Right
            string_Printf("\r\nTurning right\r\n");
            turn_right(sensor, TURN_ANGLE);
            break;

        case 'm': // 180-degree scan
            string_Printf("\r\nPerforming 180-degree scan\r\n");
            scan_and_detect_objects(sensor, 0, 180, 5);
            break;

        default:
            // Stop if any other key is pressed
            oi_setWheels(0, 0);
            break;
    }

    // Display sensor data for user interpretation
    oi_update(sensor);
    sprintf(msg, "\r\nSensor Data: Left Bumper=%d, Right Bumper=%d, Distance=%.2f cm\r\n",
            sensor->bumpLeft, sensor->bumpRight, sensor->distance / 10.0);
    string_Printf(msg);
}

void manual_mode(oi_t *sensor) {
    string_Printf("\r\n=== MANUAL MODE ACTIVATED ===");
    string_Printf("\r\nUse WASD to move, M to scan, T to exit\r\n");

    while (is_manual_mode) {
        char input = last_received_char;
        if (input != 0) {
            process_manual_command(sensor, input);
            last_received_char = 0; // Reset after processing
        }
        timer_waitMillis(50); // Small delay to avoid busy-waiting
    }
}


int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();
    cyBOT_init_Scan(0b0111);

    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    bool is_scanning = false;

    while (1) {
            if (command_flag_go == 1) {
                command_flag_go = 0;
                is_scanning = true;
                string_Printf("\r\nStarting scan...\r\n");
                scan_and_detect_objects(sensor_data, 0, 180, 4);
                is_scanning = false;
            }

            if (command_flag_stop == 1) {
                command_flag_stop = 0;
                string_Printf("\r\nScan stopped.\r\n");
            }

            if (command_flag_manual == 1) {
                command_flag_manual = 0; // Reset the flag
                if (is_manual_mode) {
                    manual_mode(sensor_data);
                }
            }

            timer_waitMillis(50);
        }

    oi_free(sensor_data);
}

