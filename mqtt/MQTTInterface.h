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

int mqtt_network_read(Network *, unsigned char *, int, int);
int mqtt_network_write(Network *, unsigned char *, int, int);
void mqtt_network_disconnect(Network *);
void mqtt_network_init(Network *);
void mqtt_network_clear(void);
int mqtt_network_connect(Network *, char *, char *);

#endif
