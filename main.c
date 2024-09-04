/*
    Include mqtt lib
    include openssl lib

    fd -> openssl -> mqtt
    mqtt -> openssl -> fd

    init
    deinit
    connenct
    disconnect
    write
    read

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include "config.h"
#include "modbus_task.h"
#include "mqtt_task.h"

void init_signals(void);
static void signal_handler(int);
static void cleanup(void);
void panic(const char *, ...);
struct sigaction sigact;

int main()
{
    pthread_t modbus_thread = 0;
    pthread_t mqtt_sub_thread = 0;
    pthread_t mqtt_pub_thread = 0;
    int ret = 0;

    atexit(cleanup);
    init_signals();

    // Create threads

    // MODBUS TCP thread
    modbus_task_config_t modbus_config = {MOBBUS_PLC_IP, MODBUS_PLC_PORT, MODBUS_PLC_REGISTER};

    ret = pthread_create(&modbus_thread, NULL, modbus_task, (void *)&modbus_config);
    if (ret != 0)
    {
        fprintf(stderr, "Error in creating modbus thread\n");
        return -1;
    }

    // MQTT thread
    mqtt_config_t mqtt_config = {MQTT_BROKER_IP, MQTT_BROKER_PORT};

    ret = pthread_create(&mqtt_sub_thread, NULL, mqtt_sub_task, (void *)&mqtt_config);
    if (ret != 0)
    {
        fprintf(stderr, "Error in creating mqtt sub thread\n");
        return -1;
    }

    ret = pthread_create(&mqtt_pub_thread, NULL, mqtt_pub_task, (void *)&mqtt_config);
    if (ret != 0)
    {
        fprintf(stderr, "Error in creating mqtt pub thread\n");
        return -1;
    }

    // Wait till threads are complete before main continues.
    pthread_join(modbus_thread, NULL);
    pthread_join(mqtt_sub_thread, NULL);
    pthread_join(mqtt_pub_thread, NULL);

    return 0;
}

void init_signals(void)
{
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
}

static void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        panic("\nCaught signal for Ctrl+C\n");
        break;

    default:
        break;
    }
}

void panic(const char *fmt, ...)
{
    char buf[150];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(buf, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "%s", buf);
    exit(-1);
}

void cleanup(void)
{
    sigemptyset(&sigact.sa_mask);
    /* Do any cleaning up chores here */
}
