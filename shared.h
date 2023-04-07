#ifndef _SHARED_H

#define _SHARED_H
#define SERVER_PORT 8080
#define SERVER_PORTSTR "8080"
#define SERVERIP "127.0.0.1" //"199.17.28.75"
#define SERVERNAME "ahscentos"
#define MAX_CLIENTS 5
#define BUFLEN 1024
#define MAX_RESOURCES_PER_REQUEST 2
typedef enum {
    REQUEST,
    GRANT,
    RELEASE,
    DEFAULT_MESSAGE
} MESSAGE_TYPE;

typedef struct
{
    MESSAGE_TYPE mType;
    int philosopher_id;
    int resources[MAX_RESOURCES_PER_REQUEST];
    int numResources;
} MessageDetails;
#endif