/*
 * movement.c
 *
 *  Created on: Feb 4, 2025
 *      Author: gojuru18
 */

#include <open_interface.h>
#include "movement.h"

double move_forward (oi_t *sensor_data, double distance_mm){
    double sum = 0;
    oi_setWheels(200,200);

    while (sum < distance_mm){
        oi_update(sensor_data);
        sum = sum  + sensor_data->distance;
    }
    oi_setWheels(0,0);
           return 0;
}
double move_backward (oi_t *sensor_data, double distance_mm){
    double sum = 0;
    oi_setWheels(-200,-200);

    while (sum < distance_mm){
        oi_update(sensor_data);
        sum = sum  - sensor_data->distance;
    }
    oi_setWheels(0,0);
           return 0;
}

void turn_left(oi_t *sensor_data, double degrees){
        double temp = 0;
        oi_setWheels(100, -100);
        while(temp < degrees-5 ){
            oi_update(sensor_data);
            temp += sensor_data->angle;

        }
        oi_setWheels(0,0);


    }

    void turn_right(oi_t *sensor_data, double degrees){
        double temp = 0;
            oi_setWheels(-100, 100);
            while(temp > -(degrees - 7)){
                oi_update(sensor_data);
                temp += sensor_data->angle;

            }
            oi_setWheels(0,0);


    }

