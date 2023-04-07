typedef enum
{
    INITIAL,
    THINKING,
    EATING
} P_STATE;

#define MIN_THINK_TIME 00 //ms
#define MAX_THINK_TIME 1000 //ms

#define MIN_EAT_TIME 00 //ms
#define MAX_EAT_TIME 1000 //ms

#define NO_CHOPSTICKS_DELAY 1 //seconds

#define CHOPSTICKS_FILE "data.txt"