#include <stdlib.h>

#include <pthread.h>

#include "cerver/socket.h"
#include "cerver/cerver.h"
#include "cerver/client.h"

Socket *socket_new (void) {

    Socket *socket = (Socket *) malloc (sizeof (Socket));
    if (socket) {
        socket->sock_fd = -1;
        socket->packet_buffer = NULL;
        socket->packet_buffer_size = 0;
        socket->mutex = NULL;
    }

    return socket;

}

void socket_delete (void *socket_ptr) {

    if (socket_ptr) {
        Socket *socket = (Socket *) socket_ptr;

        pthread_mutex_lock (socket->mutex);

        if (socket->packet_buffer) free (socket->packet_buffer);

        pthread_mutex_unlock (socket->mutex);
        pthread_mutex_destroy (socket->mutex);
        free (socket->mutex);

        free (socket_ptr);
    }

}

Socket *socket_create (int fd) {

    Socket *socket = socket_new ();
    if (socket) {
        socket->sock_fd = fd;

        socket->mutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
        pthread_mutex_init (socket->mutex, NULL);
    }

    return socket;

}

Socket *socket_get_by_fd (Cerver *cerver, int sock_fd, bool on_hold) {

    Socket *retval = NULL;

    if (cerver) {
        if (on_hold) {
            Connection *connection = connection_get_by_sock_fd_from_on_hold (cerver, sock_fd);
            if (connection) {
                retval = connection->socket;
            }
        }

        else {
            Client *client = client_get_by_sock_fd (cerver, sock_fd);
            if (client) {
                Connection *connection = connection_get_by_sock_fd_from_client (client, sock_fd);
                if (connection) {
                    retval = connection->socket;
                }
            }
        }
    }

    return retval;

}