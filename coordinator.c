#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "shared.h"

#define MAX_REQUESTS 100

typedef struct
{
    int sd;
    int num_resources;
    int resources[MAX_RESOURCES_PER_REQUEST];
    int philosopher_id;
} QueueStruct;

QueueStruct request_queue[MAX_REQUESTS];

//int request_queue[MAX_REQUESTS];
int queue_size;
bool resources[MAX_CLIENTS];
int num_eating;
int num_requests_granted[MAX_CLIENTS];

bool resourcesAvailable(int* resources_requested, int num_requested)
{
    int i;
    bool available = true;

    //printf("coordinator: checking available resources...\n");
    for (i = 0; i < num_requested; i++)
    {
        if (!resources[resources_requested[i]])
        {
            available = false;
        }
        //printf("\tresource %d -> %s\n", resources_requested[i], resources[resources_requested[i]] == 1 ? "TRUE" : "FALSE");
    }

    return available;
}

int addToQueue(int socket, int* resources_requested, int num_requested, int philosopher_id)
{
    if (queue_size >= MAX_REQUESTS)
    {
        printf("coordinator: error! Too many requests! Unable to queue request from socket = %d\n", socket);
        exit(1);
    }
    QueueStruct data; // = malloc(sizeof(QueueStruct));
    int i;

    data.sd = socket;
    data.num_resources = num_requested;
    for (i = 0; i < num_requested; i++)
    {
        data.resources[i] = resources_requested[i];
    }
    data.philosopher_id = philosopher_id;

    request_queue[queue_size] = data;
    queue_size++;
    printf("coordinator: added socket %d to queue, new size is %d\n", socket, queue_size);

    return queue_size;
}

QueueStruct getNextInQueue()
{
    QueueStruct data;
    QueueStruct foundData;
    bool found = false;
    int index = 0;
    foundData.sd = -1;
    foundData.num_resources = 0;

    while (index < queue_size)
    {
        if (!found)
        {
            data = request_queue[index];
            if (resourcesAvailable(data.resources, data.num_resources))
            {
                found = true;
                index--;
                queue_size--;
                foundData = data;
            }
        }
        else
        {
            //once found, we just shift every other element down one.
            request_queue[index] = request_queue[index+1];
        }
        index++;
    }
    //int socket;
    /*
    if (queue_size == 0)
    {
        //socket = -1;
        //data = NULL;
        data.sd = -1;
        data.num_resources = 0;
    }
    else
    {
        //socket = request_queue[queue_size-1];
        data = request_queue[queue_size-1];
        //socket = data.sd;
        //request_queue[queue_size-1] = -1;
        queue_size--;
    }
    */

    return foundData;
}


int releaseResources(int* resources_requested, int num_requested)
{
    int i;

    for (i = 0; i < num_requested; i++)
    {
        if (resources[resources_requested[i]])
            printf("coordinator: logical error!! Freeing resource %d, but it is already free!\n", resources_requested[i]);
        
        resources[resources_requested[i]] = true;
        printf("coordinator: resource %d available again!\n", resources_requested[i]);
    }

    num_eating--;
}

int grantPermission(int socket, int* resources_requested, int num_requested)
{
    //char buffer[BUFLEN+1];
    int i;
    int bytes_sent;
    MESSAGE_TYPE action = GRANT;
    bool available = resourcesAvailable(resources_requested, num_requested);
    if (!available)
    {
        printf("coordinator: error! Tried to grant permission while resources are being used!\n");
        exit(1);
    }

    for (i = 0; i < num_requested; i++)
    {
        resources[resources_requested[i]] = false;
        printf("coordinator: resource %d is now locked!\n", resources_requested[i]);
    }
    
    bytes_sent = send(socket, &action, sizeof(action), 0);

    printf("coordinator: granting permission to socket = %d\n", socket);
    num_eating++;
    
    return bytes_sent;
}

/*
MESSAGE_TYPE getMessageType(char* msg)
{
    if (strcasecmp(msg, "release") == 0)
    {
        return RELEASE;
    }
    else if (strcasecmp(msg, "grant") == 0)
    {
        return GRANT;
    }
    else if (strcasecmp(msg, "request") == 0)
    {
        return REQUEST;
    }

    return DEFAULT_MESSAGE;
}
*/

void coordinatorLoop(int master_socket, struct sockaddr_in mAddr) //int* client_sockets, int num_sockets)
{
    int addrlen;
    int client_socket[MAX_CLIENTS];
    int new_socket;
    int i;
    int u;
    int max_sd;
    int sd;
    int activity;
    int bytes_read;
    int bytes_sent;
    int next_socket;
    int num_connections = 0;
    bool active = true;
    char buffer[BUFLEN+1];
    char status_output[1024];
    int status_length;
    fd_set readfds;

    //MESSAGE_TYPE actionEnum;
    MessageDetails msgDetails;
    QueueStruct nextRequest;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client_socket[i] = 0;
    }

    addrlen = sizeof(mAddr);

    while (active)
    {
        printf("Loading status report...\n");
        memset(status_output, 0, sizeof(buffer));
        
        status_length = 0;
        status_length+=sprintf(status_output+status_length,"coordinator:\n");
        status_length+=sprintf(status_output+status_length,"----------------CURRENT STATUS---------------\n");
        status_length+=sprintf(status_output+status_length,"\t time = %s \n\t resources:\n", "DO LATER");
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            status_length+=sprintf(status_output+status_length,"\t\t resource %d = %s\n", i, resources[i] == 1 ? "FREE" : "LOCKED");
        }
        status_length+=sprintf(status_output+status_length,"\t philosophers: %d/%d\n", num_connections, MAX_CLIENTS);
        status_length+=sprintf(status_output+status_length,"\t\t eating = %d\n\t\t thinking = %d\n\t\t queue = %d\n", num_eating, num_connections - num_eating, queue_size);
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            status_length+=sprintf(status_output+status_length, "\t\t requests granted\n\t\t\t philosopher %d = %d times\n", i, num_requests_granted[i]);
        }

        status_length+=sprintf(status_output+status_length,"---------------------------------------------\n");
        printf("%s", status_output);

        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            sd = client_socket[i];

            if (sd > 0)
                FD_SET(sd, &readfds);
            
            if (sd > max_sd)
                max_sd = sd;
        }

        printf("coordinator: calling select...\n");
        activity = select(max_sd+1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
        {
            printf("coordinator: select error\n");
            exit(1);
        }
        printf("coordinator: select finished. \n");
        if (FD_ISSET(master_socket, &readfds))
        {
            new_socket = accept(master_socket, (struct sockaddr*) &mAddr, (socklen_t*)&addrlen);
            if (new_socket < 0)
            {
                printf("coordinator: accept error\n");
                exit(1);
            }

            printf("coordinator: new connection with fd = %d, ip = %s, port = %d\n", new_socket, inet_ntoa(mAddr.sin_addr), ntohs(mAddr.sin_port));
            num_connections++;

            i = 0;
            while (client_socket[i] != 0)
            {
                i++;
            }
            client_socket[i] = new_socket;
            printf("coordinator: added new socket at index = %d\n", i);
        }
        else
        {
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                sd = client_socket[i];
                if (sd > 0 && FD_ISSET(sd, &readfds))
                {
                    //bytes_read = read(sd, buffer, BUFLEN);
                    //bytes_read = read(sd, &actionEnum, sizeof(actionEnum));
                    bytes_read = read(sd, &msgDetails, sizeof(msgDetails));
                    if (bytes_read < 0)
                    {
                        perror("coordinator: error trying to read data!");
                        exit(1);
                    }
                    else if (bytes_read == 0)
                    {
                        //disconnection
                        getpeername(sd, (struct sockaddr*)&mAddr, (socklen_t*)&addrlen);
                        printf("coordinator: Host disconnected, ip %s, port %d \n", inet_ntoa(mAddr.sin_addr), ntohs(mAddr.sin_port));
                        num_connections--;

                        close(sd);
                        client_socket[i] = 0;
                    }
                    else
                    {
                        //buffer[bytes_read] = '\0';
                        //printf("coordinator: Message was: %s\n", buffer);
                        //MESSAGE_TYPE msgType = getMessageType(buffer);

                        switch (msgDetails.mType) //(msgType)
                        {
                            case REQUEST:
                                printf("coordinator: received REQUEST from socket = %d, philosopher = %d\n", sd, msgDetails.philosopher_id);
                                if (resourcesAvailable(msgDetails.resources, msgDetails.numResources))
                                {
                                    bytes_sent = grantPermission(sd, msgDetails.resources, msgDetails.numResources);
                                    if (bytes_sent <= 0)
                                    {
                                        perror("coordinator: error granting permission to socket");
                                        exit(1);
                                    }
                                    num_requests_granted[msgDetails.philosopher_id]++;
                                }
                                else
                                {
                                    printf("coordinator: resources locked right now, adding philosopher %d to queue...\n", msgDetails.philosopher_id);
                                    printf("\tlocked resources -> ");
                                    for (u = 0; u < msgDetails.numResources; u++)
                                    {
                                        printf("%d ", msgDetails.resources[u]);
                                        if (u == msgDetails.numResources-1)
                                            printf("\n");
                                        else
                                            printf(" and ");
                                    }
                                    addToQueue(sd, msgDetails.resources, msgDetails.numResources, msgDetails.philosopher_id);
                                }
                                break;
                            case RELEASE:
                                printf("coordinator: received RELEASE from socket = %d, philosopher = %d\n", sd, msgDetails.philosopher_id);
                                releaseResources(msgDetails.resources, msgDetails.numResources);
                                //next_socket = getNextSocket();
                                nextRequest = getNextInQueue();
                                if (nextRequest.sd < 0)
                                {
                                    //no socket waiting in queue
                                    printf("coordinator: no socket in queue. Nice.\n");
                                }
                                else
                                {
                                    printf("coordinator: next in queue is socket = %d and philosopher = %d, granting permissions now.\n", nextRequest.sd, nextRequest.philosopher_id);
                                    num_requests_granted[nextRequest.philosopher_id]++;
                                    grantPermission(nextRequest.sd, nextRequest.resources, nextRequest.num_resources);
                                    
                                }
                                break;
                            default:
                                break;
                        }

                    }
                }
            }
        }
    }

    printf("coordinator: finished!\n");
}

void initialize()
{
    int i;

    /*
    for (i = 0; i < MAX_REQUESTS; i++)
    {
        request_queue[i] = -1;
    }
    */

    queue_size = 0;
    num_eating = 0;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        resources[i] = true;
        num_requests_granted[i] = 0;
    }
}

int main(int argc, char* argv[])
{
    initialize();

    struct sockaddr_in sAddr;
    //struct sockaddr_in cAddr[MAX_CLIENTS];
    int server_socket;
    //int client_sockets[MAX_CLIENTS];
    //bool opt = true;
    int opt = 1;
    int i;
    int err;
    
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0); //);
    if (server_socket == -1)
    {
        perror("coordinator: socket creation failed");
        exit(1);
    }

    err = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if (err == -1)
    {
        perror("coordinator: error setsockopt");
        exit(1);
    }

    memset(&sAddr, 0, sizeof(struct sockaddr_in));
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sAddr.sin_port = htons(SERVER_PORT);

    err = bind(server_socket, (struct sockaddr *)&sAddr, sizeof(struct sockaddr_in));
    if (err == -1)
    {
        perror("coordinator: bind address to socket failed");
        exit(1);
    }

    err = listen(server_socket, MAX_CLIENTS);
    if (err == -1)
    {
        perror("coordinator: listen failed");
        exit(1);
    }

    /*
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        clientSockets[i] = accept(serverSocket, (struct sockaddr *) &cAddr[i], sizeof(struct sockaddr_in));
        if (clientSockets[i] == -1)
        {
            perror("coordinator: accept failed");
            exit(1);
        }
    }

    printf("coordinator: Accepted all client sockets!\n");
    */

    coordinatorLoop(server_socket, sAddr); //clientSockets, MAX_CLIENTS);

    printf("coordinator: exiting...\n");
    exit(0);
}