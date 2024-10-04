#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mqtt_task.h"
#include "modbus_task.h"
#include "MQTTInterface.h"
#include "MQTTClient.h"
#include "utilis.h"
#include "config.h"

#define MAX_CONN_ATTEMPS (10)
#define MQTT_BUFSIZE (1 * 1024)

Network mqttNet;
MQTTClient mqttClient;

static unsigned char sndBuffer[MQTT_BUFSIZE]; // mqtt send buffer
static unsigned char rcvBuffer[MQTT_BUFSIZE]; // mqtt receive buffer
static unsigned char msgBuffer[MQTT_BUFSIZE]; // mqtt message buffer

/**
 * @brief Callback function invoked when an MQTT message is received.
 * 
 * This function is called whenever a new MQTT message arrives. It processes
 * the incoming message data provided in the msg argument.
 * 
 * @param[in] msg Pointer to the received message data.
 * 
 * @retval None
 */
void MqttMessageArrived(MessageData *msg)
{
    MQTTMessage *message = msg->message;

    // Clear the message buffer and copy the received payload into it.
    memset(msgBuffer, 0, sizeof(msgBuffer));
    memcpy(msgBuffer, message->payload, message->payloadlen);

    // Log the received message payload and its length.
    fprintf(stdout, "[MQTT_SUB_TASK] INFO: MQTT MSG[%d]:%s\n", (int)message->payloadlen, msgBuffer);
}

/**
 * @brief Connects to an MQTT broker and subscribes to a specified topic.
 * 
 * This function establishes a connection to the MQTT broker using the provided
 * configuration and subscribes to a predefined topic.
 * 
 * @param[in] config Pointer to the MQTT configuration structure.
 * 
 * @retval MQTT_SUCCESS on successful connection and subscription.
 * @retval MQTT error code on failure.
 */
int MqttConnectBroker(mqtt_config_t* config)
{
    // Connect to MQTT broker
    int ret = mqtt_network_connect(&mqttNet, 
                                    config->broker_ip, 
                                    config->broker_port,
                                    config->ca_cert_file,
                                    config->client_cert_file,
                                    config->client_key_file);

    if (ret != MQTT_SUCCESS)
    {
        // Handle network connection failure.
        fprintf(stderr, "\n[MQTT_PUB_TASK] ERROR: ConnectNetwork failed.\n");
        mqtt_network_disconnect(&mqttNet);
        return -1;
    }

    // Initialize the MQTT client
    MQTTClientInit(&mqttClient, &mqttNet, 1000, sndBuffer, sizeof(sndBuffer), rcvBuffer, sizeof(rcvBuffer));

    // Set up MQTT connection parameters
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Linux client";
    // data.username.cstring = "roger";
    // data.password.cstring = "password";
    data.keepAliveInterval = 60;
    data.cleansession = 1;

    ret = MQTTConnect(&mqttClient, &data);
    if (ret != MQTT_SUCCESS)
    {
        // Handle MQTT connection failure
        fprintf(stderr, "[MQTT_PUB_TASK] ERROR: MQTTConnect failed.\n");
        MQTTCloseSession(&mqttClient);
        mqtt_network_disconnect(&mqttNet);
        return ret;
    }

    // Subscribe to the desired topic
    ret = MQTTSubscribe(&mqttClient, config->topic, QOS0, MqttMessageArrived);
    if (ret != MQTT_SUCCESS)
    {
        // Handle subscription failure
        fprintf(stderr, "[MQTT_PUB_TASK] ERROR: MQTTSubscribe failed.\n");
        MQTTCloseSession(&mqttClient);
        mqtt_network_disconnect(&mqttNet);
        return ret;
    }

    fprintf(stdout, "[MQTT_PUB_TASK] INFO: MQTT_ConnectBroker O.K.\n");
    return MQTT_SUCCESS;
}

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
void *mqtt_sub_task(void *arg)
{
    mqtt_config_t *config = (mqtt_config_t *)arg;

    if (config == NULL)
    {
        // No config
        fprintf(stderr, "mqtt_sub_task: Error no config!\n");
        pthread_exit(NULL);
        return NULL;
    }

    for (;;)
    {
        // If disconnected from MQTT broker fast blink the green led
        if (!mqttClient.isconnected)
        {
            sleep_ms(50);
            continue;
        }

        // Handle timer and incoming messages
        MQTTYield(&mqttClient, 500); /* Don't wait too long if no traffic is incoming */
        sleep_ms(100);
    }

    return NULL;
}

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
void *mqtt_pub_task(void *arg)
{
    mqtt_config_t *config = (mqtt_config_t *)arg;

    if (config == NULL)
    {
        // No config
        fprintf(stderr, "mqtt_pub_task: Error no config!\n");
        pthread_exit(NULL);
        return NULL;
    }

    for (;;)
    {

        // 1) Attempt to establish the connection with the MQTT broker:
        // - If connection fails, frees resources and retry.
        // - If the connection is successful, sends MQTT messages.

        int ret = FAILURE;
        int nAttemps = 0;
        do
        {
            ret = MqttConnectBroker(config);

            if (ret != MQTT_SUCCESS)
            {
                fprintf(stderr, "mqtt_pub_task: [%d] Error connecting to broker retry...\n", nAttemps++);
            }
        } while (ret != MQTT_SUCCESS && nAttemps < 10);

        if (ret != MQTT_SUCCESS)
        {
            fprintf(stderr, "mqtt_pub_task: Error connecting to broker\n");
            return NULL;
        }

        int error = 0;
        char str[256] = {0};
        MQTTMessage message;
        unsigned long ulNotifiedValue = 0;

        do
        {
            // Wait for plc data
            pthread_mutex_lock(&mutex);
            // Attendi un nuovo valore
            while (pthread_cond_wait(&cond, &mutex) != 0)
                ;
            ulNotifiedValue = shared_value;
            pthread_mutex_unlock(&mutex);

            // Composing the message to be sent
            snprintf(str, sizeof(str),
                     "{\n"
                     "  \"device\": \"%s\",\n"
                     "  \"data\": %lu\n"
                     "}",
                     config->device_name,
                     ulNotifiedValue);
            
            message.payload = (void *)str;
            message.payloadlen = strlen(str);

            // Send the message at topic
            if (MQTTPublish(&mqttClient, config->topic, &message) != MQTT_SUCCESS)
            {
                MQTTCloseSession(&mqttClient);
                mqtt_network_disconnect(&mqttNet);
                error = 1;
                continue;
            }

            // Wait
            sleep_ms(1000);

        } while (mqttClient.isconnected && !error);
    }

    return NULL;
}
