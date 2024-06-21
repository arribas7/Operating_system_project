
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>

// ----- DEFINITIONS -----

// - Generic IO Functions

int generic_interface_wait(int time, t_config* config) 
{
    int work_unit_time = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    int time_to_wait = work_unit_time * time;

    return usleep(time_to_wait);
}