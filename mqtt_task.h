#ifndef __MQTT_TASK_H__
#define __MQTT_TASK_H__

/**
 * @brief Structure holding the MQTT client configuration parameters.
 * 
 * This structure contains all the necessary information required to connect 
 * to an MQTT broker, including the broker's IP address, port, device details, 
 * and security certificates.
 */
typedef struct
{
    char *broker_ip;          /**< IP address of the MQTT broker */
    char *broker_port;        /**< Port number of the MQTT broker */
    char *device_name;        /**< Name of the device connecting to the broker */
    char *topic;              /**< Topic to subscribe or publish to */
    char *ca_cert_file;       /**< Path to the CA certificate file for TLS */
    char *client_cert_file;   /**< Path to the client's certificate file for TLS */
    char *client_key_file;    /**< Path to the client's private key file for TLS */
} mqtt_config_t;


/**
 * @brief Function implementing the MqttClientSubTask thread.
 * 
 * This function runs as a separate thread and handles the MQTT client's
 * subscription-related tasks, including message handling and topic management.
 * It continuously listens for incoming MQTT messages on subscribed topics.
 * 
 * @param[in] arg Pointer to arguments required by the thread (if any).
 * 
 * @retval Pointer to the result of the thread execution or NULL.
 */
void *mqtt_sub_task(void *arg);

/**
 * @brief Function implementing the MqttClientPubTask thread.
 * 
 * This function runs as a separate thread and handles the MQTT client's
 * publishing tasks. It is responsible for sending messages to specified 
 * MQTT topics based on the application's needs.
 * 
 * @param[in] arg Pointer to arguments required by the thread (if any).
 * 
 * @retval Pointer to the result of the thread execution or NULL.
 */
void *mqtt_pub_task(void *arg);

#endif