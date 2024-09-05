#include "MQTTInterface.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// Specifica il percorso al certificato CA (adatta in base alla tua situazione)
// #define CA_CERT_FILE "./ca.crt" // Percorso relativo
// #define CA_CERT_FILE "/etc/ssl/certs/ca-certificates.crt" // Percorso assoluto
// #define CA_CERT_FILE getenv("CA_CERT_PATH") // Variabile d'ambiente

#define CA_CERT_FILE "cert/ca.crt"		   // Percorso al certificato CA del server
#define CLIENT_CERT_FILE "cert/client.crt" // Percorso al certificato del client
#define CLIENT_KEY_FILE "cert/client.key"  // Percorso alla chiave privata del client

int sockfd, ret;
struct sockaddr_in server_addr; // Nome variabile più descrittivo
struct hostent *server;
SSL_CTX *ctx;
SSL *ssl;
BIO *bio_err;

void mqtt_network_init(Network *n)
{
	n->socket = 0;							 // clear
	n->mqttread = mqtt_network_read;		 // receive function
	n->mqttwrite = mqtt_network_write;		 // send function
	n->disconnect = mqtt_network_disconnect; // disconnection function
}

int mqtt_network_connect(Network *n, char *ip, char *port)
{
	mqtt_network_init(n);
	// mqtt_network_clear();

	// Inizializza OpenSSL
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	fprintf(stdout, "%s\n", SSLeay_version(OPENSSL_VERSION));

	// Inizializza bio_err
	bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);

	// Crea il contesto SSL
	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
	{
		fprintf(stderr, "Errore nella creazione del contesto SSL\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Carica il certificato CA del server
	if (SSL_CTX_load_verify_locations(ctx, CA_CERT_FILE, NULL) != 1)
	{
		fprintf(stderr, "Errore nel caricamento del certificato CA\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Carica il certificato e la chiave privata del client
	if (SSL_CTX_use_certificate_file(ctx, CLIENT_CERT_FILE, SSL_FILETYPE_PEM) != 1)
	{
		fprintf(stderr, "Errore nel caricamento del certificato client\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_KEY_FILE, SSL_FILETYPE_PEM) != 1)
	{
		fprintf(stderr, "Errore nel caricamento della chiave privata client\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Verifica che la chiave privata corrisponda al certificato
	if (SSL_CTX_check_private_key(ctx) != 1)
	{
		fprintf(stderr, "Errore: la chiave privata non corrisponde al certificato\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Carica i certificati di root CA di sistema (opzionale ma consigliato)
	if (SSL_CTX_set_default_verify_paths(ctx) != 1)
	{
		fprintf(stderr, "Avviso: errore nel caricamento dei certificati CA di sistema (potrebbe non essere un problema se stai utilizzando un certificato CA personalizzato)\n");
		ERR_print_errors_fp(stderr);
		// Non terminare il programma, poiché potresti comunque essere in grado di verificare il certificato utilizzando il certificato CA specifico
	}

	// Crea il socket TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("Errore nella creazione del socket");
		return 1;
	}

	// Risolve l'hostname
	server = gethostbyname(ip);
	if (server == NULL)
	{
		fprintf(stderr, "Errore nella risoluzione dell'host %s\n", ip);
		return 1;
	}

	// Imposta l'indirizzo del server
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	int int_port = atoi(port);
	server_addr.sin_port = htons(int_port);

	// Connetti al server
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Errore nella connessione");
		return 1;
	}

	// Crea la struttura SSL e associa il socket
	ssl = SSL_new(ctx);
	if (!ssl)
	{
		fprintf(stderr, "Errore nella creazione della struttura SSL\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}
	SSL_set_fd(ssl, sockfd);

	// Abilita i messaggi di debug
	// SSL_set_info_callback(ssl, apps_ssl_info_callback);

	// Effettua l'handshake SSL
	ret = SSL_connect(ssl);
	if (ret != 1)
	{
		fprintf(stderr, "Errore nell'handshake SSL\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Verifica il certificato del server
	if (SSL_get_verify_result(ssl) != X509_V_OK)
	{
		fprintf(stderr, "Errore nella verifica del certificato del server\n");
		ERR_print_errors_fp(stderr); // Stampa gli errori di OpenSSL per ottenere maggiori dettagli
		return 1;
	}

	return 0;
}

int mqtt_network_read(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
	int read = 0;
	int total_read = 0;

	while (total_read < len)
	{
		read = SSL_read(ssl, buffer + total_read, len - total_read);
		if (read <= 0)
		{
			int err = SSL_get_error(ssl, read);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
			{
				// La lettura è stata bloccata, riprova più tardi
				continue;
			}
			else
			{
				fprintf(stderr, "Errore nella lettura dei dati dal server: %s\n", ERR_error_string(ERR_get_error(), NULL));
				return -1; // Indica un errore
			}
		}
		total_read += read;
	}

	return total_read; // Restituisce il numero totale di byte letti
}

int mqtt_network_read_(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
	int read = 0;
	int total_read = 0;
	struct timeval tv;
	fd_set readfds;

	(void)n;

	while (total_read < len)
	{
		// Imposta il timeout
		tv.tv_sec = timeout_ms / 1000;			 // Secondi
		tv.tv_usec = (timeout_ms % 1000) * 1000; // Microsecondi

		// Inizializza il file descriptor set
		FD_ZERO(&readfds);
		FD_SET(n->socket, &readfds);

		// Attendi che il socket sia pronto per la lettura o che scada il timeout
		int rc = select(n->socket + 1, &readfds, NULL, NULL, &tv);

		if (rc < 0)
		{
			// Errore in select
			perror("Error in select");
			return -1;
		}
		else if (rc == 0)
		{
			// Timeout scaduto
			return 0; // Indica un timeout
		}
		else
		{
			// Socket pronto per la lettura
			read = SSL_read(ssl, buffer + total_read, len - total_read); // Assumiamo che n->ssl sia il puntatore all'oggetto SSL
			if (read <= 0)
			{
				int err = SSL_get_error(ssl, read);
				if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				{
					// La lettura è stata bloccata, riprova più tardi
					continue;
				}
				else
				{
					fprintf(stderr, "Errore nella lettura dei dati dal server: %s\n", ERR_error_string(ERR_get_error(), NULL));
					return -1; // Indica un errore
				}
			}
			total_read += read;
		}
	}

	return total_read; // Restituisce il numero totale di byte letti
}

int mqtt_network_write(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
	int written = 0;
	int total_written = 0;

	(void)n;
	(void)timeout_ms;

	while (total_written < len)
	{
		written = SSL_write(ssl, buffer + total_written, len - total_written);
		if (written <= 0)
		{
			int err = SSL_get_error(ssl, written);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
			{
				// La scrittura è stata bloccata, riprova più tardi
				continue;
			}
			else
			{
				fprintf(stderr, "Errore nella scrittura dei dati al server: %s\n", ERR_error_string(ERR_get_error(), NULL));
				return -1; // Indica un errore
			}
		}
		total_written += written;
	}

	return total_written; // Restituisce il numero totale di byte scritti
}

void mqtt_network_disconnect(Network *n)
{
	(void)n;

	// Chiudi la connessione SSL e il socket
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	// Libera bio_err alla fine del programma
	BIO_free(bio_err);
	SSL_CTX_free(ctx);
}

void mqtt_network_clear()
{
	// Chiudi la connessione SSL e il socket
	//	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	// Libera bio_err alla fine del programma
	BIO_free(bio_err);
	SSL_CTX_free(ctx);
}

// Timer functions
void TimerInit(Timer *timer)
{
	timer->end_time = (struct timeval){0, 0};
}

char TimerIsExpired(Timer *timer)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}

void TimerCountdownMS(Timer *timer, unsigned int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
	timeradd(&now, &interval, &timer->end_time);
}

void TimerCountdown(Timer *timer, unsigned int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timeval interval = {timeout, 0};
	timeradd(&now, &interval, &timer->end_time);
}

int TimerLeftMS(Timer *timer)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&timer->end_time, &now, &res);
	// printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
	return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}
