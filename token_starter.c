#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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
        execl("./token_philosopher", "token_philosopher", id_str, (char *) NULL);
    }
    
    return childId;
}

int main(int argc, char* argv[])
{
    pid_t childId;
    pid_t wpid;
    int status = 0;
    int i;

    for (i = 0; i < 5; i++)
    {
        childId = startPhilosopher(i);
        printf("Philosopher %d started! Pid = %d\n", i, childId);
    }
    printf("All child processes started.\n");

    while ((wpid = wait(&status)) > 0);

    printf("All child processes finished.\n");

    exit(0);
}