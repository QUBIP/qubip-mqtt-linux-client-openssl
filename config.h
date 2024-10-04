#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct
{
    char *ca_cert_file;
    char *client_cert_file;
    char *client_key_file;
    char *modbus_plc_ip;
    char *modbus_plc_port;
    int modbus_plc_register;
    char *mqtt_broker_ip;
    char *mqtt_broker_port;
    char *mqtt_device_name;
    char *mqtt_topic;
} config_t;

#endif