#include <stdio.h>
#include <stdlib.h>
#include "modbus_task.h"
#include "nanomodbus.h"
#include "platform.h"
#include "config.h"
#include "utilis.h"

#define MAX_CONN_ATTEMPS 10

unsigned long shared_value = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// This task read a holding register, send it to mqtt task, increment and send it back to the plc.

// Connect
// connected ? no -> connect
// connected
// read register
// send task notification to mqtt task
// increment plc data
// send new data to plc
// check if i need to disconnect and exit

void *modbus_task(void *arg)
{
    modbus_task_config_t *config = (modbus_task_config_t *)arg;

    if(config == NULL)
    {
        // No config
        fprintf(stderr, "modbus_task: Error no config!\n");
        pthread_exit(NULL);
        return NULL;
    }

    int nAttemps = 0;
    void *conn = NULL;
    do
    {
        // Set up the TCP connection
        conn = connect_tcp(config->plc_ip, config->plc_port);
        if (!conn)
        {
            fprintf(stderr, "modbus_task: [%d] Error connecting to server retry...\n", nAttemps++);
        }
        else
        {
            break;
        }
    } while (nAttemps < MAX_CONN_ATTEMPS);

    if (!conn)
    {
        fprintf(stderr, "modbus_task: Error connecting to server\n");
        return NULL;
    }

    nmbs_platform_conf platform_conf;
    platform_conf.transport = NMBS_TRANSPORT_TCP;
    platform_conf.read = read_fd_linux;
    platform_conf.write = write_fd_linux;
    platform_conf.arg = conn; // Passing our TCP connection handle to the read/write functions

    // Create the modbus client
    nmbs_t nmbs;
    nmbs_error err = nmbs_client_create(&nmbs, &platform_conf);
    if (err != NMBS_ERROR_NONE)
    {
        fprintf(stderr, "modbus_task: Error creating modbus client\n");
        if (!nmbs_error_is_exception(err))
        {
            return NULL;
        }
    }

    // Set only the response timeout. Byte timeout will be handled by the TCP connection
    nmbs_set_read_timeout(&nmbs, 1000);

    do
    {
        // Read holding register
        uint16_t r_regs[1];
        err = nmbs_read_holding_registers(&nmbs, config->plc_register, 1, r_regs);
        if (err != NMBS_ERROR_NONE)
        {
            fprintf(stderr, "modbus_task: Error reading holding register at address %d - %s\n", config->plc_register, nmbs_strerror(err));
            if (!nmbs_error_is_exception(err))
            {
                disconnect(conn);
                continue; // check while condition
            }
        }

        printf("modbus_task: Register at address %d: %d\n", config->plc_register, r_regs[0]);

        // Send register to mqtt task
        pthread_mutex_lock(&mutex);
        shared_value = r_regs[0];
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        //printf("Send data [%d] to mqtt task", r_regs[0]);

        // Increment data
        ++r_regs[0];

        // Write data in holding register
        uint16_t w_regs[1] = {r_regs[0]};
        err = nmbs_write_multiple_registers(&nmbs, config->plc_register, 1, w_regs);
        if (err != NMBS_ERROR_NONE)
        {
            fprintf(stderr, "modbus_task: Error writing register at address %d - %s\n", config->plc_register, nmbs_strerror(err));
            if (!nmbs_error_is_exception(err))
            {
                disconnect(conn);
                continue; // check while condition
            }
        }

        // wait some time
        sleep_ms(1000);

        // check if an error occurred or i need disconnect
    } while (err == NMBS_ERROR_NONE && 1);

    // Close the TCP connection
    disconnect(conn);

    // No need to destroy the nmbs instance, bye bye
    return NULL;
}
