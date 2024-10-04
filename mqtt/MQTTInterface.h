#ifndef __MQTT_INTERFACE_H__
#define __MQTT_INTERFACE_H__
/* Private includes ----------------------------------------------------------*/
#include <sys/time.h>

/* Private define ------------------------------------------------------------*/
/* CONFIG */

#if defined MQTT_INTERFACE_DEBUG
#define MQTT_INTERFACE_DEBUG_LOG(message, ...) DEBUG_LOG(message, ##__VA_ARGS__)
#else
#define MQTT_INTERFACE_DEBUG_LOG(message, ...)
#endif

/* Typedef -----------------------------------------------------------*/
typedef struct
{
	struct timeval end_time;
} Timer;

typedef struct Network Network;

struct Network
{
	int socket;
	int (*mqttread)(Network *, unsigned char *, int, int);
	int (*mqttwrite)(Network *, unsigned char *, int, int);
	void (*disconnect)(Network *);
};

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void TimerInit(Timer *);
char TimerIsExpired(Timer *);
void TimerCountdownMS(Timer *, unsigned int);
void TimerCountdown(Timer *, unsigned int);
int TimerLeftMS(Timer *);

/**
 * @brief Initializes the network structure for MQTT communication.
 * 
 * This function sets up the provided Network structure for use in MQTT communication.
 * It configures the network interface and prepares it for connection to the MQTT broker.
 * 
 * @param[in,out] n Pointer to the Network structure to be initialized.
 * 
 * @retval None
 */
void mqtt_network_init(Network *);

/**
 * @brief Establishes a network connection to the MQTT broker.
 * 
 * This function connects to the MQTT broker at the specified IP address and port, 
 * using TLS with the provided certificate files for secure communication.
 * 
 * @param[in,out] n Pointer to the Network structure representing the network connection.
 * @param[in] ip The IP address of the MQTT broker.
 * @param[in] port The port number of the MQTT broker.
 * @param[in] ca_cert_file Path to the CA certificate file for TLS validation.
 * @param[in] client_cert_file Path to the client certificate file for TLS authentication.
 * @param[in] client_key_file Path to the client's private key file for TLS authentication.
 * 
 * @retval 0 on successful connection.
 * @retval Non-zero error code on failure.
 */
int mqtt_network_connect(Network *, char *, char *, char *, char *, char *);

/**
 * @brief Reads data from the MQTT network connection.
 * 
 * This function reads a specified number of bytes from the network connection
 * into the provided buffer. It blocks for the specified timeout duration if data 
 * is not immediately available.
 * 
 * @param[in] n Pointer to the Network structure representing the connection.
 * @param[out] buffer Pointer to the buffer where the received data will be stored.
 * @param[in] len The maximum number of bytes to read.
 * @param[in] timeout_ms The timeout period in milliseconds to wait for data.
 * 
 * @retval The number of bytes successfully read, or a negative error code on failure.
 */
int mqtt_network_read(Network *, unsigned char *, int, int);

/**
 * @brief Writes data to the MQTT network connection.
 * 
 * This function sends the specified number of bytes from the buffer over the network 
 * connection. It will attempt to write the data within the given timeout period.
 * 
 * @param[in] n Pointer to the Network structure representing the connection.
 * @param[in] buffer Pointer to the buffer containing the data to be sent.
 * @param[in] len The number of bytes to send from the buffer.
 * @param[in] timeout_ms The timeout period in milliseconds to complete the write operation.
 * 
 * @retval The number of bytes successfully written, or a negative error code on failure.
 */
int mqtt_network_write(Network *, unsigned char *, int, int);

/**
 * @brief Disconnects the MQTT network connection.
 * 
 * This function gracefully closes the connection to the MQTT broker and 
 * performs any necessary cleanup of the network resources.
 * 
 * @param[in,out] n Pointer to the Network structure representing the connection.
 * 
 * @retval None
 */
void mqtt_network_disconnect(Network *);

/**
 * @brief Clears and resets the network resources for MQTT communication.
 * 
 * This function clears any allocated resources and resets the internal state 
 * of the network interface used for MQTT communication. It prepares the network 
 * structure for reuse or reinitialization.
 * 
 * @retval None
 */
void mqtt_network_clear(void);


#endif
