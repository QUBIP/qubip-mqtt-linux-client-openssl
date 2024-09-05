#ifndef __CONFIG_H__
#define __CONFIG_H__

// #define MOBBUS_PLC_IP           "192.168.101.212"
#define MOBBUS_PLC_IP           "192.168.168.133"
//#define MODBUS_PLC_PORT         "502"
#define MODBUS_PLC_PORT         "5002"
//#define MODBUS_PLC_REGISTER     (32770)
#define MODBUS_PLC_REGISTER     (10)

#define MQTT_BROKER_IP          "192.168.101.63"
#define MQTT_BROKER_PORT        "1883"

#define MQTT_TOPIC              "2023/test"


// Specifica il percorso al certificato CA (adatta in base alla tua situazione)
// #define CA_CERT_FILE "./ca.crt" // Percorso relativo
// #define CA_CERT_FILE "/etc/ssl/certs/ca-certificates.crt" // Percorso assoluto
// #define CA_CERT_FILE getenv("CA_CERT_PATH") // Variabile d'ambiente

#define CA_CERT_FILE "cert/ca.crt"		   // Percorso al certificato CA del server
#define CLIENT_CERT_FILE "cert/client.crt" // Percorso al certificato del client
#define CLIENT_KEY_FILE "cert/client.key"  // Percorso alla chiave privata del client

#endif