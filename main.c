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
#include <getopt.h>
#include <string.h>
#include "config.h"
#include "modbus_task.h"
#include "mqtt_task.h"

void load_config_from_command_line_args(int argc, char *argv[], config_t *config);
void init_signals(void);
static void signal_handler(int);
static void cleanup(void);
void panic(const char *, ...);
struct sigaction sigact;


int main(int argc, char *argv[])
{
    pthread_t modbus_thread = 0;
    pthread_t mqtt_sub_thread = 0;
    pthread_t mqtt_pub_thread = 0;
    config_t config;
    int ret = 0;

    atexit(cleanup);
    init_signals();

    // Read and parse arguments
    load_config_from_command_line_args(argc, argv, &config);

    // Create threads

    // MODBUS TCP thread
    modbus_task_config_t modbus_config = {config.modbus_plc_ip, config.modbus_plc_port, config.modbus_plc_register};

    ret = pthread_create(&modbus_thread, NULL, modbus_task, (void *)&modbus_config);
    if (ret != 0)
    {
        fprintf(stderr, "Error in creating modbus thread\n");
        return -1;
    }

    // MQTT thread
    mqtt_config_t mqtt_config = {
        config.mqtt_broker_ip,
        config.mqtt_broker_port,
        config.mqtt_device_name,
        config.mqtt_topic,
        config.ca_cert_file,
        config.client_cert_file,
        config.client_key_file};

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

/**
 * @brief Loads configuration settings from command-line arguments.
 * 
 * Parses the command-line arguments and populates the provided configuration 
 * structure with the relevant values.
 * 
 * @param[in] argc The number of command-line arguments.
 * @param[in] argv Array of argument strings.
 * @param[out] config Pointer to the configuration structure to be populated.
 * 
 * @retval None
 */
void load_config_from_command_line_args(int argc, char *argv[], config_t *config)
{
    int opt = 0;
    const char *short_opts = "";

    // Define long options
    static struct option long_opts[] = {
        {"ca-cert", required_argument, 0, '1'}, // or no_argument
        {"client-cert", required_argument, 0, '2'},
        {"client-key", required_argument, 0, '3'},
        {"modbus-ip", required_argument, 0, '4'},
        {"modbus-port", required_argument, 0, '5'},
        {"modbus-register", required_argument, 0, '6'},
        {"mqtt-broker", required_argument, 0, '7'},
        {"mqtt-port", required_argument, 0, '8'},
        {"mqtt-device-name", required_argument, 0, '9'},
        {"mqtt-topic", required_argument, 0, 'a'},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
    {
        switch (opt)
        {
        case '1': // CA_CERT_FILE
            config->ca_cert_file = optarg;
            break;
        case '2': // CLIENT_CERT_FILE
            config->client_cert_file = optarg;
            break;
        case '3': // CLIENT_KEY_FILE
            config->client_key_file = optarg;
            break;
        case '4': // MODBUS_PLC_IP
            config->modbus_plc_ip = optarg;
            break;
        case '5': // MODBUS_PLC_PORT
            config->modbus_plc_port = optarg;
            break;
        case '6': // MODBUS_PLC_REGISTER
            config->modbus_plc_register = atoi(optarg);
            break;
        case '7': // MQTT_BROKER_IP
            config->mqtt_broker_ip = optarg;
            break;
        case '8': // MQTT_BROKER_PORT
            config->mqtt_broker_port = optarg;
            break;
        case '9': // MQTT_DEVICE_NAME
            config->mqtt_device_name = optarg;
            break;
        case 'a': // MQTT_TOPIC
            config->mqtt_topic = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s --ca-cert CA_CERT_FILE --client-cert CLIENT_CERT_FILE --client-key CLIENT_KEY_FILE --modbus-ip MODBUS_PLC_IP --modbus-port MODBUS_PLC_PORT --modbus-register MODBUS_PLC_REGISTER --mqtt-broker MQTT_BROKER_IP --mqtt-port MQTT_BROKER_PORT --mqtt-topic MQTT_TOPIC\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Check if all parameters is set
    if (!config->ca_cert_file ||
        !config->client_cert_file ||
        !config->client_key_file ||
        !config->modbus_plc_ip ||
        !config->modbus_plc_port ||
        !config->modbus_plc_register ||
        !config->mqtt_broker_ip ||
        !config->mqtt_broker_port ||
        !config->mqtt_device_name ||
        !config->mqtt_topic)
    {
        fprintf(stderr, "Missing required arguments\n");
        exit(EXIT_FAILURE);
    }
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
