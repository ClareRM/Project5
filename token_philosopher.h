#include <stdbool.h>

#define SERVER_PORT_BASE 54320
#define SERVER_PORTSTR "54320"
#define SERVERIP "127.0.0.1" //"199.17.28.75"
#define SERVERNAME "my_computer"
#define MAX_CLIENTS 5
#define BUFLEN 1024
//#define MAX_RESOURCES_PER_REQUEST 2

#define MIN_THINK_TIME 1000 //ms
#define MAX_THINK_TIME 5000 //ms

#define MIN_EAT_TIME 1000 //ms
#define MAX_EAT_TIME 5000 //ms

#define NO_CHOPSTICKS_DELAY 1 //seconds

typedef enum
{
    INITIAL,
    THINKING,
    EATING,
    WAITING
} P_STATE;

typedef struct
{
    bool chopsticks[5];
    int expectedId;
    int lastId;
    P_STATE status[5];
    int wait_times[5];
    bool changed;
} TOKEN;

/*
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
*/