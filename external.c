#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <math.h>
#include <string.h>


// the data between processes 
typedef struct 
{
    int quit;
    float temp;
} TempData;

typedef struct 
{
    long type;
    TempData data;
} Message;

float temp = 0.0f;
int selfnum = 0;
int selfqid = -1;
int parentnum = 0;
int childsnum = 0;
int parentqid = -1;
int childsqid[2];    // at most 2 children 

void Usage()
{
    fprintf(stderr, "./external <id> <initial-temperature>\n");
    exit(0);
}

int GetMsgQueue(int number, int create)
{
    key_t key = ftok("/", number);
    int qid = -1;
    if (create)
        qid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    else 
        qid = msgget(key, 0666);
    return qid;
}

int IsNumber(char* str)
{
    for (int i = 0; i < (int)strlen(str); ++ i)
    {
        if (str[i] == '.' || (str[i] >= '0' && str[i] <= '9'))
            continue;
        return 0;
    }
    return 1;
}

void InitQueues()
{
    parentnum = (selfnum == 0) ? 0 : 1;
    childsnum = (selfnum >  2) ? 0 : 2;
    
    selfqid = GetMsgQueue(selfnum, 1);
    if (selfqid < 0)
    {
        perror("msgget IPC_EXCEL");
        exit(-1);
    }
    
    int childs[2] = {0};
    int parent = 0;
    switch (selfnum)
    {
    case 0: childs[0] = 1; childs[1] = 2; break;
    case 1: childs[0] = 3; childs[1] = 4; parent = 0; break;
    case 2: childs[0] = 5; childs[1] = 6; parent = 0; break;
    case 3: parent = 1; break;
    case 4: parent = 1; break;
    case 5: parent = 2; break;
    case 6: parent = 2; break;
    }

    // create the message queue to children processes 
    for (int i = 0; i < childsnum; ++ i)
    {
        childsqid[i] = GetMsgQueue(childs[i], 0);
        if (childsqid[i] < 0)
        {
            perror("msgget");
            exit(-1);
        }
    }
    // wait till parent create successfully 
    while (parentnum != 0 && parentqid <= 0)
    {
        parentqid = GetMsgQueue(parent, 0);
        usleep(500);
    }
}

void MainLoop()
{
    float difftemp = 100.0f;
    
    while (1)
    {
        int quit = 0;
        Message msg;
        
        /* try to read from parent */
        if (parentnum != 0)
        {
            msgrcv(selfqid, &msg, sizeof(TempData), 0, 0);
            if (msg.data.quit)
                quit = 1;
            else 
                temp = (temp + msg.data.temp) / 2;
        }
        else 
        {
            if (difftemp < 0.01f)
                quit = 1;
        }

        /* try to update to children */
        msg.data.quit = quit;
        msg.data.temp = temp;
        for (int i = 0; i < childsnum; ++ i)
            msgsnd(childsqid[i], &msg, sizeof(TempData), 0);
        
        if (quit)
        {
            printf("Process #%d: final temperature %.3f\n", selfnum, temp);
            break;
        }
        
        /* try to wait all children */
        float sumf = 0.0f;
        if (childsnum > 0)
        {
            for (int i = 0; i < childsnum; ++ i)
            {
                msgrcv(selfqid, &msg, sizeof(TempData), 0, 0);
                sumf += msg.data.temp;
            }
            float origtemp = temp;
            temp = (temp + sumf) / 3;
            printf("Process #%d: current temperature %.3f\n", selfnum, temp);
            difftemp = fabs(origtemp - temp);
        }

        /* try to send parent */
        msg.data.quit = 0;
        msg.data.temp = temp;
        if (parentnum != 0)
            msgsnd(parentqid, &msg, sizeof(TempData), 0);
    }
}

void UninitQueues()
{
    msgctl(selfqid, IPC_RMID, NULL);
}

int main(int argc, char* argv[])
{
    if (argc != 3 || !IsNumber(argv[1]) || !IsNumber(argv[2]))
        Usage();
    
    selfnum = atoi(argv[1]);
    if (selfnum < 0 || selfnum > 6)
        Usage();
    temp = atof(argv[2]);

    InitQueues();
    MainLoop();
    UninitQueues();
    
    return 0;
}

