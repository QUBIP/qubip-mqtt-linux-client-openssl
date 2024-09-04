#ifndef __MQTT_TASK_H__
#define __MQTT_TASK_H__

typedef struct
{
    char *broker_ip;
    char *broker_port;
} mqtt_config_t;

void *mqtt_sub_task(void *arg);
void *mqtt_pub_task(void *arg);

#endif