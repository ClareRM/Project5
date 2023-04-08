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
#include <sys/time.h>
#include <errno.h>

//#include "shared.h"
#include "token_philosopher.h"

P_STATE currentState;
int pid;
int philosopher_id;
int left_id;
int right_id;
//MessageDetails msgDetails;
TOKEN token;

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

void printTokenInfo()
{
    char status_output[1024];
    char* status;
    int status_length;
    int status_t;
    int i;
    time_t current_t;
    struct tm* time_info;

    time(&current_t);
    time_info = localtime(&current_t);
    
    memset(status_output, 0, sizeof(status_output));
    status_length = 0;
    
    status_length+=sprintf(status_output+status_length,"philosopher (%d): status of token has changed!!\n", pid);
    status_length+=sprintf(status_output+status_length,"----------------TOKEN STATUS---------------\n");
    status_length+=sprintf(status_output+status_length,"\t%s\n", asctime(time_info));
    for (i = 0; i < 5; i++)
    {
        status_length+=sprintf(status_output+status_length,"\tChopstick %d = \t%s\n", i, token.chopsticks[i] ? "FREE" : "TAKEN");
    }

    for (i = 0; i < 5; i++)
    {
        status = "NA";
        switch (token.status[i])
        {
            case THINKING:
                status = "THINKING";
                break;
            case EATING:
                status = "EATING";
                break;
            case WAITING:
                status = "WAITING";
                break;
            case INITIAL:
                status = "INITIAL";
                break;
            default:
                break;
        }
        status_length+=sprintf(status_output+status_length,"\tStatus %d = %s\t(%ld seconds)\n", i, status, current_t - token.wait_times[i]);
    }
    status_length+=sprintf(status_output+status_length,"-------------------------------------------\n");

    printf("%s", status_output);
}

bool passToken(int socket, struct timeval* time_out)
{
    TOKEN newToken;
    fd_set fd;
    bool result;
    int err;
    int i;
    int activity;

    FD_ZERO(&fd);
    FD_SET(socket, &fd);
    activity = select(socket + 1, NULL, &fd, NULL, time_out);
    if (activity == -1)
    {
        //TO-DO: Error message
        exit(1);
    }

    if (activity > 0)
    {
        result = true;
        //printf("Passing token to next process!\n");
        newToken.lastId = philosopher_id;
        newToken.expectedId = right_id;
        newToken.changed = token.changed;
        for (i = 0; i < 5; i++)
        {
            newToken.chopsticks[i] = token.chopsticks[i];
            newToken.status[i] = token.status[i];
            newToken.wait_times[i] = token.wait_times[i];
        }
        if (token.changed)
            printTokenInfo();
        err = send(socket, &newToken, sizeof(TOKEN), 0);
        if (err == -1)
        {
            printf("philosopher (%d): error passing token to next philosopher! \n\t Error: %s\n", pid, strerror(errno));
            exit(1);
        }

        //printf("philosopher (%d): successfully passed token from philosopher %d to philosopher %d, fd = %d \n", pid, philosopher_id, right_id, socket);
    }
    else
    {
        result = false;
        printf("philosopher (%d): unable to pass token from philosopher %d to philosopher %d - TIME_OUT\n", pid, philosopher_id, right_id);
    }

    return result;
}

bool receiveToken(int socket, struct timeval* time_out)
{
    TOKEN newToken;
    fd_set fd;
    bool result;
    int err;
    int i;
    int activity;

    FD_ZERO(&fd);
    FD_SET(socket, &fd);
    //printf("philosopher (%d): addr = %p, timeout values = %ld and %ld\n", pid, time_out, time_out->tv_sec, time_out->tv_usec);
    activity = select(socket + 1, &fd, NULL, NULL, time_out);
    if (activity == -1)
    {
        //TO-DO: Error message
        exit(1);
    }
    //sleep(1);

    if (activity > 0)
    {
        result = true;
        //printf("philosopher (%d): waiting to receive token...\n", pid);
        err = recv(socket, &newToken, sizeof(TOKEN), 0);
        if (err == -1)
        {
            printf("philosopher (%d): error receiving token.\n\tError: %s\n", pid, strerror(errno));
            exit(1);
        }

        
        if (newToken.expectedId == philosopher_id)
        {
            token.expectedId = newToken.expectedId;
            token.lastId = newToken.lastId;
            for (i = 0; i < 5; i++)
            {
                token.chopsticks[i] = newToken.chopsticks[i];
                token.status[i] = newToken.status[i];
                token.wait_times[i] = newToken.wait_times[i];
            }
            token.changed = false;
        }
        else
        {
            printf("philosopher (%d): received token not meant for it! Calling receiveToken() again...\n", pid);
            result = receiveToken(socket, time_out);
        }
    }
    else
    {
        result = false;
        //printf("philosopher (%d): unable to receive token - TIME_OUT\n", pid);
    }

    return result;
}

bool takeChopsticks()
{
    bool result = false;
    char status_output[1024];
    int status_length;
    int i;

    memset(status_output, 0, sizeof(status_output));
    status_length = 0;
    
    for (i = 0; i < 5; i++)
    {
        //status_length+=sprintf(status_output+status_length,"\tChopstick %d = %s\n", i, token.chopsticks[i] ? "free" : "missing");
    }
    
    if (token.chopsticks[left_id] && token.chopsticks[right_id])
    {
        token.chopsticks[left_id] = false;
        token.chopsticks[right_id] = false;
        token.changed = true;
        result = true;
        //status_length+=sprintf(status_output+status_length,"\tSuccess!\n");
        status_length+=sprintf(status_output+status_length,"philosopher (%d): grabbed chopsticks %d and %d!\n", pid, left_id, right_id);
    }
    else
    {
        //status_length+=sprintf(status_output+status_length,"\tFailure!\n");
        status_length+=sprintf(status_output+status_length,"philosopher (%d): can't grab chopsticks %d and %d!\n", pid, left_id, right_id);
    }

    printf("%s", status_output);

    return result;
}

void returnChopsticks()
{
    char status_output[1024];
    int status_length;
    int i;

    memset(status_output, 0, sizeof(status_output));
    status_length = 0;
    status_length+=sprintf(status_output+status_length,"philosopher (%d): returning chopsticks %d and %d!\n", pid, left_id, right_id);
    for (i = 0; i < 5; i++)
    {
        //status_length+=sprintf(status_output+status_length,"\tChopstick %d = %s\n", i, token.chopsticks[i] ? "free" : "missing");
    }
    printf("%s", status_output);
    if (token.chopsticks[left_id] || token.chopsticks[right_id])
    {
        printf("philosopher (%d): Error! Somehow tried to return chopsticks, but they are already: left = %d, right = %d\n",
            pid,
            token.chopsticks[left_id],
            token.chopsticks[right_id]
        );
    }

    token.chopsticks[left_id] = true;
    token.chopsticks[right_id] = true;
    token.changed = true;
}

void philosopherLoop(int client_socket, int server_socket)
{
    bool active = true;
    int err;
    int delay_seconds;
    int activity;
    fd_set fd;
    bool switched;
    bool result;
    bool took_chopsticks;
    struct timeval wait_time;
    wait_time.tv_sec = 1;
    wait_time.tv_usec = 0;

    time_t current_t;
    time_t goal_t = 0;
    time_t hungry_t = 0;

    bool hasToken = false;
    if (philosopher_id == 0)
        hasToken = true;

    while (active)
    {
        time(&current_t);
        switched = false;

        if (goal_t - current_t <= 0)
        {
            //We have finished our elapsed time...
            //time to switch actions!
            switched = true;
            switch (currentState)
            {
                case THINKING:
                    printf("philosopher (%d): finished thinking.\n", pid);
                    //printf("philosopher (%d): thinking for %d seconds\n", pid, delay_seconds);
                    currentState = EATING;
                    //sleep(delay_seconds);
                    //printf("philosopher (%d): finished thinking, now trying to eat...\n", pid);
                    //startEating(client_socket, server_socket);
                    break;
                case EATING:
                    printf("philosopher (%d): finished eating.\n", pid);
                    //printf("philosopher (%d): eating for %d seconds\n", pid, delay_seconds);
                    
                    currentState = THINKING;
                    //sleep(delay_seconds);
                    //printf("philosopher (%d): finished eating, putting chopsticks back\n", pid);
                    //stopEating(client_socket, server_socket);
                    break;
                case WAITING:
                    //printf("philosopher (%d): waiting for chopsticks, hungry time = %ld seconds\n", pid, (current_t - hungry_t));
                    break;
                case INITIAL:
                    //printf("philosopher (%d): currently in initial state\n", pid);
                    break;
                default:
                    break;
            }
        }
        
        /*
        If we are waiting...
            If hasToken == false, then
                Wait 1 second to see if we can receive token
                If yes then
                    receive token
                    hasToken = true
            if hasToken == true, then
                Wait 1 second to see if we can pass token.
                If yes then
                    pass token
                    hasToken = false
        
        Otherwise if we switched...
            If not hasToken, then
                wait until we get token
            Modify token data (error if taken chopsticks are already free)
            Wait until we can pass token
            Pass token
            If eating now, then...
                Print that we are eating for # seconds
            else...
                Print that we are thinking for # seconds
            Set goal_t for # seconds after present

        */
        
        if (switched)
        {
            //changing to a new state

            //we may want to change the delay later, so set it here
            
            

            
            if (!hasToken)
            {
                //printf("philosopher (%d): waiting to receive token from socket = %d\n", pid, server_socket);
                //wait_time.tv_sec = 1;
                    //printf("philosopher (%d): addr = %p, wait_time values = %ld and %ld\n", pid, &wait_time, wait_time.tv_sec, wait_time.tv_usec);
                receiveToken(server_socket, NULL);
                hasToken = true;
            }
            //printf("philosopher (%d): currently has token\n", pid);

            switch(currentState)
            {
                case THINKING:
                    returnChopsticks();
                    break;
                case EATING:
                case WAITING:
                    //printf("Calling takeChopsticks()\n");
                    if (!takeChopsticks())
                    {
                        currentState = WAITING;
                    }
                    else
                    {
                        hungry_t = 0;
                        currentState = EATING;
                    }
                    break;
                case INITIAL:
                    currentState = THINKING;
                default:
                    //error message
                    break;
            }
            if (token.status[philosopher_id] != currentState)
            {
                token.changed = true;
                token.wait_times[philosopher_id] = current_t;
            }
            token.status[philosopher_id] = currentState;

            result = passToken(client_socket, NULL);
            if (result)
                hasToken = false;
            
            delay_seconds = getRandomDelay(currentState);
            switch (currentState)
            {
                case THINKING:
                    printf("philosopher (%d): thinking for %d seconds\n", pid, delay_seconds);
                    goal_t = current_t + delay_seconds;
                    break;
                case EATING:
                    printf("philosopher (%d): eating for %d seconds\n", pid, delay_seconds);
                    goal_t = current_t + delay_seconds;
                    break;
                case WAITING:
                    if (hungry_t == 0)
                    {
                        time(&hungry_t);
                        printf("philosopher (%d): couldn't get chopsticks. Waiting...\n", pid);
                    }   
                    
                    sleep(1);
                

                    break;
                default:
                    //error
                    break;
            }
        }
        else if (hasToken)
        {
            wait_time.tv_sec = 1;
            result = passToken(client_socket, &wait_time);
            if (result)
                hasToken = false;
        }
        else
        {
            wait_time.tv_sec = 1;
            result = receiveToken(server_socket, &wait_time);
            if (result)
                hasToken = true;
        }
    }

    printf("philosopher (%d): has died", pid);
}

void initialize(int argc, char* argv[])
{
    int i;

    currentState = INITIAL;
    pid = getpid();
    if (argc == 2)
    {
        philosopher_id = atoi(argv[1]);
        left_id = philosopher_id;
        right_id = philosopher_id + 1;

        //if (left_id < 0)
        //    left_id = MAX_CLIENTS;
        if (right_id >= MAX_CLIENTS)
            right_id = 0;
    }
    else
    {
        printf("philosopher (%d): error! expected 2 arguments when calling this process.\n", pid);
        exit(1);
    }

    //msgDetails.numResources = 2;
    //msgDetails.resources[0] = left_id;
    //msgDetails.resources[1] = right_id;
    //msgDetails.philosopher_id = philosopher_id;
    token.expectedId = philosopher_id;
    token.lastId = right_id;
    token.changed = true;
    
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        token.chopsticks[i] = true;
        token.status[i] = INITIAL;
        token.wait_times[i] = time(NULL);
    }

    srand(time(NULL) + pid + philosopher_id);



    //printf("philosopher (%d): initialized!\n", pid);
}

void initializeSockets(int *server_socket, int *client_socket, struct sockaddr_in *server_addr, struct sockaddr_in *client_addr)
{
    const int SERVER_PORT = SERVER_PORT_BASE + philosopher_id;
    const int CLIENT_PORT = SERVER_PORT_BASE + right_id;
    int err;
    int opt = 1;

    //INITIALIZE SERVER SOCKET
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_socket == -1)
    {
        perror("philosopher: socket creation failed");
        exit(1);
    }

    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr->sin_port = htons(SERVER_PORT);
    //printf("philosopher (%d): id = %d, client_port = %d, server_port = %d\n", pid, philosopher_id, CLIENT_PORT, SERVER_PORT);

    err = setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if (err == -1)
    {
        perror("coordinator: error setsockopt");
        exit(1);
    }

    err = bind(*server_socket, (struct sockaddr *) server_addr, sizeof(*server_addr));
    if (err == -1)
    {
        printf("philosopher (%d): bind address to socket failed. id = %d, server_port = %d\n\tError: %s\n", pid, philosopher_id, SERVER_PORT, strerror(errno));
        //perror("philosopher: bind address to socket failed");
        exit(1);
    }

    err = listen(*server_socket, MAX_CLIENTS);
    if (err == -1)
    {
        perror("philosopher: listen failed");
        exit(1);
    }
    //END INITIALIZE SERVER SOCKET

    //INITIALIZE CLIENT SOCKET
    memset(client_addr, 0, sizeof(*client_addr));
    client_addr->sin_family = AF_INET;
    client_addr->sin_port = htons (CLIENT_PORT);
    client_addr->sin_addr.s_addr = inet_addr(SERVERIP);

    *client_socket = socket ( AF_INET, SOCK_STREAM,0);// | SOCK_NONBLOCK, 0);
    if (*client_socket == -1) {
        printf ("philosopher (%d): socket creation failed\n", pid);
        perror("\tError");
        exit (1);
    }
    //END INITIALIZE CLIENT SOCKET
}

void setupSockets(int* server_socket, int* client_socket, struct sockaddr_in server_addr, struct sockaddr_in client_addr)
{
    fd_set write_fds;
    struct timeval waitTime;
    int err;
    int new_socket;
    int server_len = sizeof(server_addr);
    int activity;
    char* buffer;
    int opt_results;
    int opt;
    int opt_len;
    int val;
    socklen_t val_len;

    waitTime.tv_sec = 0;
    waitTime.tv_usec = 0;
    
    FD_ZERO(&write_fds);
    //FD_SET(client_socket, &write_fds);

    
    //wait for client_socket to be listening
    val_len = sizeof(val);
    do
    {
        err = getsockopt(*client_socket, SOL_SOCKET, SO_ACCEPTCONN, &val, &val_len);
        if (err == -1)
        {
            printf("philosopher (%d): connect failed.\n\tError: %s\n", pid, strerror(errno));
            exit (1);
        }
        sleep(1);
    } while (val != 0);
    //printf("philosopher (%d): found listening socket at fd = %d.\n", pid, client_socket);

    //client_socket is non-blocking, so we expect either success or EINPROGRESS
    err = connect (*client_socket, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in));
    if (err == -1 && errno != EINPROGRESS)
    {
        printf("philosopher (%d): connect failed.\n\tError: %s\n", pid, strerror(errno));
        exit (1);
    }

    //now that we tried connecting, we want to set so we can check with select() later.
    FD_SET(*client_socket, &write_fds);

    new_socket = accept(*server_socket, (struct sockaddr*) &server_addr, (socklen_t*) &server_len);
    if (new_socket < 0)
    {
        printf("philosopher (%d): accept error.\n\tError: %s\n", pid, strerror(errno));
        exit(1);
    }
    *server_socket = new_socket;
    printf("philosopher (%d): Accepted connection from fd = %d\n", pid, new_socket);

    
    activity = select(*client_socket+1, NULL, &write_fds, NULL, NULL);
    if (activity < 0)
    {
        printf("philosopher (%d): select error: %s\n", pid, strerror(errno));
        exit(1);
    }

    
    opt_len = sizeof(int);
    opt_results = getsockopt(*client_socket, SOL_SOCKET, SO_ERROR, &opt, &opt_len);
    if (opt_results == -1)
    {
        printf("philosopher (%d): getsockopt error\n", pid);
        perror("\tError:");
        exit(1);
    }
    //printf("philosopher (%d): getsockopt result = %s\n", pid, strerror(opt));
    
    

    //we should now be ready!
    //printf("philosopher (%d): connected!\n", pid);

}

int main(int argc, char* argv[])
{
    initialize(argc, argv);
    int server_socket;
    int client_socket; 
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    //printf("philosopher (%d): initializing sockets\n", pid);

    initializeSockets(
        &server_socket,
        &client_socket,
        &server_addr,
        &client_addr
    );

    //printf("philosopher (%d): done initializing sockets\n", pid);

    setupSockets(
        &server_socket,
        &client_socket,
        server_addr,
        client_addr
    );

    printf("philosopher (%d): FINISHED ALL INITIALIZATION\n", pid);
    

    philosopherLoop(client_socket, server_socket);

    printf("philosopher (%d): quitting\n", pid);

    exit(1);
}