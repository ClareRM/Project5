#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#include "shared.h"
#include "philosopher.h"

P_STATE currentState;
int pid;
int philosopher_id;
int left_id;
int right_id;
MessageDetails msgDetails;

float getRandomDelayPrecise(P_STATE state)
{
    int delay_ms;
    switch (state)
    {
        case THINKING:
            delay_ms = MIN_THINK_TIME + rand()%(MAX_THINK_TIME-MIN_THINK_TIME);
            break;
        case EATING:
            delay_ms = MIN_EAT_TIME + rand()%(MAX_EAT_TIME-MIN_EAT_TIME);
            break;
        default:
            delay_ms = 15000;
            break;
    }

    return ((float) delay_ms)/1000.0;
}

int getRandomDelay(P_STATE state)
{
    int delay_ms;
    switch (state)
    {
        case THINKING:
            delay_ms = MIN_THINK_TIME + rand()%(MAX_THINK_TIME-MIN_THINK_TIME);
            break;
        case EATING:
            delay_ms = MIN_EAT_TIME + rand()%(MAX_EAT_TIME-MIN_EAT_TIME);
            break;
        default:
            delay_ms = 15000;
            break;
    }

    return delay_ms/1000;
}

bool takeChopsticks()
{
    //need to access some shared data...
    
    FILE* finput;
    bool chopsticks[MAX_CLIENTS];
    bool result;
    int i;
    int c_val;
    int n;

    finput = fopen(CHOPSTICKS_FILE, "r+");

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        c_val = 0;
        n = fscanf(finput, "%d ", &c_val); //NOTE: We only expect a 1 or a 0. Should be a way to use a special regex pattern to do this...probably.
        chopsticks[i] = c_val;
    }

    if (!chopsticks[left_id] || !chopsticks[right_id])
    {
        printf("philosopher (%d): mistake? We somehow ended up in a situation where we have access to resources, but no chopsticks!\n", pid);
        //exit(1);
        result = false;
    }
    else
    {
        chopsticks[left_id] = false;
        chopsticks[right_id] = false;

        fseek(finput, 0, SEEK_SET);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            fprintf(finput, "%d ", chopsticks[i]);
        }
        result = true;
    }
    
    
    fclose(finput);

    return result;
}

void returnChopsticks()
{
    FILE* finput;
    bool chopsticks[MAX_CLIENTS];
    int i;
    int c_val;
    int n;

    finput = fopen(CHOPSTICKS_FILE, "r+");

    //NOTE: This method involves reading, making two changes, and then printing again.
    //No big deal for such a small file, but...consider rewriting to only
    //change data at correct locations?
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        c_val = 0;
        n = fscanf(finput, "%d ", &c_val); //NOTE: We only expect a 1 or a 0. Should be a way to use a special regex pattern to do this...probably.
        chopsticks[i] = c_val;

        /*
        if (feof(finput))
        {
            printf("philosopher (%d): end-of-file detected on file %s\n", pid, CHOPSTICKS_FILE);
            exit(1);
        }
        */
    }

    if (chopsticks[left_id] || chopsticks[right_id])
    {
        printf("philosopher (%d): error! We somehow ended up in a situation where we tried to put chopsticks back, but they were already there!\n", pid);
        exit(1);
    }
    
    chopsticks[left_id] = true;
    chopsticks[right_id] = true;

    fseek(finput, 0, SEEK_SET);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        fprintf(finput, "%d ", chopsticks[i]);
    }
    
    fclose(finput);
}

void stopEating(int socket)
{
    int err;
    MESSAGE_TYPE actionEnum;

    msgDetails.mType = RELEASE;
    err = send(socket, &msgDetails, sizeof(msgDetails), 0);
    if (err == -1)
    {
        printf("philosopher (%d): error sending data to coordinator.\n", pid);
        exit(1);
    }

    printf("philosopher (%d): let coordinator know to release data. Now thinking.\n", pid);

    currentState = THINKING;

    /*
    MESSAGE_TYPE requestEnum = REQUEST;
    MESSAGE_TYPE releaseEnum = RELEASE;

    err = send(socket, &requestEnum, sizeof(requestEnum), 0);
    if (err == -1)
    {
        printf("philosopher (%d): error sending data to coordinator.\n", pid);
        exit(1);
    }

    printf("philosopher (%d): sent request for resources to coordinator...\n", pid);
    err = recv(socket, &actionEnum, sizeof(actionEnum), 0);
    if (err == -1)
    {
        printf("philosopher (%d): error receiving data to coordinator.\n", pid);
        exit(1);
    }

    printf("philosopher (%d): received response from coordinator\n", pid);
    if (GRANT != actionEnum)
    {
        printf("philosopher (%d): error! GRANT = %d but received %d\n", pid, GRANT, actionEnum);
        exit(1);
    }

    printf("philosopher (%d): with approval from coordinator, now trying to return chopsticks\n", pid);
    returnChopsticks();
    printf("philosopher (%d): chopsticks returned\n", pid);

    err = send(socket, &releaseEnum, sizeof(releaseEnum), 0);
    if (err == -1)
    {
        printf("philosopher (%d): error sending data to coordinator.\n", pid);
        exit(1);
    }
    printf("philosopher (%d): let coordinator know to release data.\n", pid);
    */
    
    /*
    printf("philosopher (%d): need access to data again!\n", pid);

    printf("philosopher (%d): \n", pid);
    printf("philosopher (%d): \n", pid);
    printf("philosopher (%d): \n", pid);
    */
}

void startEating(int socket)
{
    int err;
    MESSAGE_TYPE actionEnum;
    //NOTE: Rather inefficient to allocate memory when using it...
    //There should be a better way than defining a variable.
    //MESSAGE_TYPE requestEnum = REQUEST;
    //MESSAGE_TYPE releaseEnum = RELEASE;
    bool result;

    msgDetails.mType = REQUEST;
    //err = send(socket, &requestEnum, sizeof(requestEnum), 0);
    err = send(socket, &msgDetails, sizeof(msgDetails), 0);
    if (err == -1)
    {
        printf("philosopher (%d): error sending data to coordinator.\n", pid);
        exit(1);
    }

    printf("philosopher (%d): sent request for resources to coordinator...\n", pid);
    err = recv(socket, &actionEnum, sizeof(actionEnum), 0);
    if (err == -1)
    {
        printf("philosopher (%d): error receiving data to coordinator.\n", pid);
        exit(1);
    }

    printf("philosopher (%d): received response from coordinator\n", pid);
    if (GRANT != actionEnum)
    {
        printf("philosopher (%d): error! GRANT = %d but received %d\n", pid, GRANT, actionEnum);
        exit(1);
    }

    printf("philosopher (%d): with approval from coordinator, now eating\n", pid);
    currentState = EATING;

    /*
    result = takeChopsticks();
    if (result)
    {
        printf("philosopher (%d): chopsticks taken\n", pid);
        msgDetails.mType = RELEASE;
        err = send(socket, &msgDetails, sizeof(msgDetails), 0);
        if (err == -1)
        {
            printf("philosopher (%d): error sending data to coordinator.\n", pid);
            exit(1);
        }
        printf("philosopher (%d): let coordinator know to release data.\n", pid);

        currentState = EATING;
    }
    else
    {
        printf("philosopher (%d): no available chopsticks. Trying again in %d seconds.\n", pid, NO_CHOPSTICKS_DELAY);
        sleep(NO_CHOPSTICKS_DELAY);
        startEating(socket);
    }
    */
}

void philosopherLoop(int clientSocket)
{
    bool active = true;
    int err;
    int delay_seconds;
    MESSAGE_TYPE actionEnum;

    while (active)
    {
        char msg[BUFLEN];
        delay_seconds = getRandomDelay(currentState);
        switch (currentState)
        {
            case THINKING:
                printf("philosopher (%d): thinking for %d seconds\n", pid, delay_seconds);
                sleep(delay_seconds);
                printf("philosopher (%d): finished thinking, now trying to eat...\n", pid);
                startEating(clientSocket);
                break;
            case EATING:
                printf("philosopher (%d): eating for %d seconds\n", pid, delay_seconds);
                sleep(delay_seconds);
                printf("philosopher (%d): finished eating, putting chopsticks back\n", pid);
                stopEating(clientSocket);
                //snprintf(msg, BUFLEN+1, "%s", "Eating");
                break;
            default:
                //snprintf(msg, BUFLEN+1, "%s", "Default");
                break;
        }
        

        //strncpy(state, "Thinking", )
        /*
        err = send(clientSocket, msg, strlen(msg), 0);
        if (err == -1)
        {
            printf("philosopher (%d): error sending data to coordinator.\n", pid);
            exit(1);
        }
        sleep(15);
        */
    }

    printf("philosopher (%d): has died", pid);
}

void initialize(int argc, char* argv[])
{
    currentState = THINKING;
    pid = getpid();
    if (argc == 2)
    {
        philosopher_id = atoi(argv[1]);
        left_id = philosopher_id;
        right_id = philosopher_id + 1;

        //if (left_id < 0)
        //    left_id = MAX_CLIENTS-1;
        if (right_id >= MAX_CLIENTS)
            right_id = 0;
    }
    else
    {
        printf("philosopher (%d): error! expected 2 arguments when calling this process.\n", pid);
        exit(1);
    }

    msgDetails.numResources = 2;
    msgDetails.resources[0] = left_id;
    msgDetails.resources[1] = right_id;
    msgDetails.philosopher_id = philosopher_id;

    srand(time(NULL) + pid + philosopher_id);

    printf("philosopher (%d): initialized!\n", pid);
}

int main(int argc, char* argv[])
{
    initialize(argc, argv);

    struct sockaddr_in sAddr;
    struct sockaddr_in cAddr;
    int err;
    int clientSocket;

    memset(&cAddr, 0, sizeof (struct sockaddr_in));
    cAddr.sin_family = AF_INET;
    cAddr.sin_port = htons (SERVER_PORT);
    cAddr.sin_addr.s_addr = inet_addr(SERVERIP);

    clientSocket = socket ( AF_INET, SOCK_STREAM, 0); //IPPROTO_TCP);
    if (clientSocket == -1) {
        printf ("philosopher (%d): socket creation failed\n", pid);
        exit (1);
    }

    err = connect (clientSocket, (struct sockaddr *)&cAddr, sizeof(struct sockaddr_in));
    if (err == -1) {
        printf("philosopher (%d): connect failed\n", pid);
        exit (1);
    }

    printf("philosopher (%d): connected!\n", pid);

    philosopherLoop(clientSocket);

    printf("philosopher (%d): quitting\n", pid);

    exit(1);
}