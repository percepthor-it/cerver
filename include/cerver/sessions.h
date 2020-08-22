#ifndef _CERVER_SESSIONS_H_
#define _CERVER_SESSIONS_H_

#include "cerver/client.h"
#include "cerver/packets.h"

struct _AuthData;

// auxiliary struct that is passed to cerver session id generator
typedef struct SessionData {

    Packet *packet;
    struct _AuthData *auth_data;
    Client *client;

} SessionData;

extern SessionData *session_data_new (Packet *packet, struct _AuthData *auth_data, Client *client);

extern void session_data_delete (void *ptr);

// create a unique session id for each client based on the current time
extern void *session_default_generate_id (const void *session_data);

#pragma region serialization

#define TOKEN_SIZE         256

// serialized session id - token
struct _SToken {

    char token[TOKEN_SIZE];

};

typedef struct _SToken SToken;

#pragma endregion

#endif