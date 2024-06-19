
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>

// Generic interface functions

/**
 * @fn generic_interface_wait
 * @brief Wait time microseconds * unit_work_time
**/ 
int generic_interface_wait(int time, t_config* config);