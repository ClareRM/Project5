#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int startCoordinator()
{
    int childId;

    childId = fork();

    if (childId == -1)
    {
        printf("starter: error creating fork\n");
        exit(1);
    }
    else if (childId == 0)
    {
        //Child
        execl("./coordinator", "coordinator", (char *) NULL);
    }
    
    return childId;
}

int startPhilosopher(int philosopher_id)
{
    int childId;

    childId = fork();

    if (childId == -1)
    {
        printf("starter: error creating fork\n");
        exit(1);
    }
    else if (childId == 0)
    {
        //Child
        int id_len = snprintf(NULL, 0, "%d", philosopher_id);
        char* id_str = malloc(id_len+1);
        snprintf(id_str, id_len+1, "%d", philosopher_id);
        execl("./philosopher", "philosopher", id_str, (char *) NULL);
    }
    
    return childId;
}

int main(int argc, char* argv[])
{
    pid_t childId;
    pid_t wpid;
    int status = 0;
    int i;


    childId = startCoordinator();
    printf("Coordinator started! Pid = %d\n", childId);

    for (i = 0; i < 5; i++)
    {
        childId = startPhilosopher(i);
        printf("Philosopher %d started! Pid = %d\n", i, childId);
    }
    printf("All child processes started.\n");

    while ((wpid = wait(&status)) > 0);

    printf("All child processes finished.\n");

    exit(0);

    /*
    if (argc == 1)
    {
        execl("./central_starter", "central_starter", '1', (char *) NULL);
    }
    else if (argc == 2)
    {
        id = atoi(argv[1]);
        
        childId = fork();
        if (childId == -1)
        {
            printf("starter: error creating fork\n");
            exit(1);
        }
        else if (childId == 0)
        {
            //child
            if (id == 1)
            {
                //execute coordinator
                execl("./coordinator", "coordinator", (char *) NULL);
            }
            else
            {
                execl("./philosopher", "philosopher", (char *) NULL);
            }
        }
        else if (childId > 0)
        {
            int numLength = snprintf(NULL, 0, "%d", id+1);
            char* newIdStr = malloc(numLength+1);
            snprintf(newIdStr, numLength+1, "%d", id+1);
            execl("./central_starter", "central_start", newIdStr, (char *) NULL);
        }
    }
    */
}