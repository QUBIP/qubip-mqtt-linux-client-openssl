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
#include "../config.h"

int sockfd, ret;
struct sockaddr_in server_addr;
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

int mqtt_network_connect(Network *n, char *ip, char *port, char* ca_cert_file, char* client_cert_file, char* client_key_file)
{
	mqtt_network_init(n);
	// mqtt_network_clear();

	// Initialize OpenSSL
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	fprintf(stdout, "%s\n", SSLeay_version(OPENSSL_VERSION));

	// Initialize bio_err
	bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);

	// Create SSL context
	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
	{
		fprintf(stderr, "Error creating SSL context\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Load the server's CA certificate
	if (SSL_CTX_load_verify_locations(ctx, ca_cert_file, NULL) != 1)
	{
		fprintf(stderr, "Error loading CA certificate\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Load the client's certificate and private key
	if (SSL_CTX_use_certificate_file(ctx, client_cert_file, SSL_FILETYPE_PEM) != 1)
	{
		fprintf(stderr, "Error loading client certificate\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, client_key_file, SSL_FILETYPE_PEM) != 1)
	{
		fprintf(stderr, "Error loading client's private key\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Verify that the private key matches the certificate
	if (SSL_CTX_check_private_key(ctx) != 1)
	{
		fprintf(stderr, "Error: private key does not match the certificate\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Load system root CA certificates (optional but recommended)
	if (SSL_CTX_set_default_verify_paths(ctx) != 1)
	{
		fprintf(stderr, "Warning: error loading system CA certificates (this may not be an issue if you're using a custom CA certificate)\n");
		ERR_print_errors_fp(stderr);
		// Do not terminate the program, as you may still be able to verify the certificate using the specific CA certificate
	}

	// Create the TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("Error creating socket");
		return 1;
	}

	// Resolve the hostname
	server = gethostbyname(ip);
	if (server == NULL)
	{
		fprintf(stderr, "Error resolving host %s\n", ip);
		return 1;
	}

	// Set the server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	int int_port = atoi(port);
	server_addr.sin_port = htons(int_port);

	// Connect to the server
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Error connecting");
		return 1;
	}

	// Create the SSL structure and associate the socket
	ssl = SSL_new(ctx);
	if (!ssl)
	{
		fprintf(stderr, "Error creating SSL structure\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}
	SSL_set_fd(ssl, sockfd);

	// Enable debug messages
	// SSL_set_info_callback(ssl, apps_ssl_info_callback);

	// Perform the SSL handshake
	ret = SSL_connect(ssl);
	if (ret != 1)
	{
		fprintf(stderr, "Error in SSL handshake\n");
		ERR_print_errors_fp(stderr);
		return 1;
	}

	// Verify the server's certificate
	if (SSL_get_verify_result(ssl) != X509_V_OK)
	{
		fprintf(stderr, "Error verifying the server's certificate\n");
		ERR_print_errors_fp(stderr); // Print OpenSSL errors for more details
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
				// The read was blocked, try again later
				continue;
			}
			else
			{
				fprintf(stderr, "Error reading data from the server: %s\n", ERR_error_string(ERR_get_error(), NULL));
				return -1; // Indicates an error
			}
		}
		total_read += read;
	}

	return total_read; // Returns the total number of bytes read
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
		// Set the timeout
		tv.tv_sec = timeout_ms / 1000;			 // Seconds
		tv.tv_usec = (timeout_ms % 1000) * 1000; // Microseconds

		// Initialize the file descriptor set
		FD_ZERO(&readfds);
		FD_SET(n->socket, &readfds);

		// Wait for the socket to be ready for reading or for the timeout to expire
		int rc = select(n->socket + 1, &readfds, NULL, NULL, &tv);

		if (rc < 0)
		{
			// Error in select
			perror("Error in select");
			return -1;
		}
		else if (rc == 0)
		{
			// Timeout expired
			return 0; // Indicates a timeout
		}
		else
		{
			// Socket ready for reading
			read = SSL_read(ssl, buffer + total_read, len - total_read); // Assuming n->ssl is the pointer to the SSL object
			if (read <= 0)
			{
				int err = SSL_get_error(ssl, read);
				if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
				{
					// The read was blocked, try again later
					continue;
				}
				else
				{
					fprintf(stderr, "Error reading data from the server: %s\n", ERR_error_string(ERR_get_error(), NULL));
					return -1; // Indicates an error
				}
			}
			total_read += read;
		}
	}

	return total_read; // Returns the total number of bytes read
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
				// The write was blocked, try again later
				continue;
			}
			else
			{
				fprintf(stderr, "Error writing data to the server: %s\n", ERR_error_string(ERR_get_error(), NULL));
				return -1; // Indicates an error
			}
		}
		total_written += written;
	}

	return total_written; // Returns the total number of bytes written
}

void mqtt_network_disconnect(Network *n)
{
	(void)n;

	// Close the SSL connection and the socket
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	// Free bio_err at the end of the program
	BIO_free(bio_err);
	SSL_CTX_free(ctx);
}

void mqtt_network_clear()
{
	// Close the SSL connection and the socket
	//	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	// Free bio_err at the end of the program
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
