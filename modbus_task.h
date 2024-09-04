#ifndef __MODBUS_TASK_H_
#define __MODBUS_TASK_H_

#include <pthread.h>

extern unsigned long shared_value;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;

typedef struct
{
    char *plc_ip;
    char *plc_port;
    unsigned int plc_register;
} modbus_task_config_t;

void *modbus_task(void *arg);

#endif