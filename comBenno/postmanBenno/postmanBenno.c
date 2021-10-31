/**
 * @file postmanBenno.c
 * @author Léo Chauvin
 * @brief postmanBenno allows to receive the initial tour data from postmanVisio and to send back the current bins data.
 * @version 2.0
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <json-c/json.h>
#include <stddef.h>
#include <errno.h>

#include "postmanBenno.h"
#include "./../dispatcherBenno/dispatcherBenno.h"
#include "./../../uiBenno/speaker/speaker.h"
#include "./../../utils.h"
#include "./../../common.h"

typedef enum{
    S_FORGET=0,     /**<Default state*/
    S_IDLE,         /**<Idle state*/
    S_WRITE,        /**<State to write data towards Visio*/
    S_DEATH,        /**<State to kill the active object*/
    NB_STATE        /**<Number of states*/
} State;

const char * stateName[] = {"S_FORGET", "S_IDLE", "S_WRITE", "S_DEATH"};
static const char * stateGetName(uint8_t i)
{
    return stateName[i];
}

typedef enum{
    A_NOP=0,                    /**<Action that does nothing*/
    A_START,                    /**<Action to initialise the listening and data sockets*/
    A_WRITE,                    /**<Action to write data towards Visio*/
    A_NOTIFY_DISCONNECTION,     /**<Action to try to reconnect*/
    A_STOP,                     /**<Action to stop the writing thread and to close the sockets*/
    NB_ACTION                   /**<Number of actions*/
} Action;
const char * actionName[] = {"A_NOP", "A_START", "A_WRITE", "A_NOTIFY_DISCONNECTION", "A_STOP"};
static const char * actionGetName(uint8_t i)
{
    return actionName[i];
}

typedef struct{
    State destinationState;
    Action action;
} Transition;

typedef enum{
    E_CONNECTED=0,      /**<Event to be ready to write data*/
    E_SEND_MSG,         /**<Event to write data to Visio*/
    E_ERROR_COM,        /**<Event to declare a communication error*/
    E_STOP,             /**<Event to trigger the destruction of the active object*/
    NB_EVENT            /**<Number of event*/
} Event;

const char * eventName[] = {"E_CONNECTED", "E_SEND_MSG", "E_ERROR_COM", "E_STOP"};
static const char * eventGetName(uint8_t i)
{
    return eventName[i];
}

typedef struct
{
	Event event;
// Generic definition to allow the copy of the events parameters
} MqMsgData;

// Trick to send or receive messages in the letterbox without worrying about their conversion in Event
typedef union
{
    MqMsgData data;
    char buffer[sizeof(MqMsgData)];
} MqMsg;

// Definition of function pointers
typedef void (*ActionPtr)();


#define MAX_PENDING_CONNECTIONS 5
/**
 * @def MQ_MAX_MESSAGES
 *
 * The maximum number of message the queue can handle at one time
 *
 * By default, cannot be higher than 10, unless you change this
 * value into /proc/sys/fs/mqueue/msg_max
 */

#define MQ_MAX_MESSAGES 10

/**
 * @brief Transition matrix representing the state machine
 * 
 */
static Transition mySm [NB_STATE][NB_EVENT] =
{
    [S_IDLE][E_CONNECTED]={S_WRITE, A_START},
    [S_IDLE][E_STOP]={S_DEATH, A_STOP},
    [S_WRITE][E_SEND_MSG]={S_WRITE, A_WRITE},
    [S_WRITE][E_ERROR_COM]={S_IDLE, A_NOTIFY_DISCONNECTION},
    [S_WRITE][E_STOP]={S_DEATH, A_STOP},
};

static State myState = S_IDLE; // Initialisation of the state

static const char queueName[24] = "/Bal_PostmanBenno_Write";
static mqd_t myBal;
static pthread_t pthreadWrite;
static pthread_t pthreadRead;
static Transition * myTransition;

static int socketListener;
static int socketData;
static bool dataToReceive;
static char receiveBuffer[MAX_BUFFER_SIZE];
static char sendBuffer[MAX_BUFFER_SIZE];

/**
 * @brief Informs the writing thread that there is a communication error
 * 
 */
static void notifyDisconnection();

/**
 * @brief Reads data from Visio
 * 
 * @param param 
 * @return void* 
 */
static void * readMsg(void * param);

#ifndef _TESTING_MODE
/**
 * @brief Change the state and execute the action
 * 
 * @param msg The MqMsg containing the event
 */
static void postmanBenno_transitionFct(MqMsg msg);
#endif

/**
 * @brief The routine of the writing pthread
 * 
 * @param param 
 * @return void* 
 */
static void * run(void * param);

/**
 * @brief To send a MqMsg to the letterbox
 * 
 * @param msg The MqMsg to send
 */
static void mqSend(MqMsg * msg);

/**
 * @brief To receive a MqMsg from the letterbox
 * 
 * @param msg The MqMsg to receive
 */
static void mqReceive(MqMsg * msg);

/**
 * @brief Action that does nothing
 * 
 */
static void actionNop();

/**
 * @brief Action that initialises the listening and data sockets
 * 
 */
static void actionStart();

/**
 * @brief Action to write data to Visio
 * 
 */
static void actionWrite();

/**
 * @brief Action to try to reconnect
 * 
 */
static void actionNotifyDisconnection();

/**
 * @brief Action to stop the writing thread and to close the sockets
 * 
 */
static void actionStop();

// Table of actions to do
static const ActionPtr actionsTab[NB_ACTION] = {&actionNop, &actionStart, &actionWrite, &actionNotifyDisconnection, &actionStop};


extern void PostmanBenno_new()
{
	struct mq_attr mqa = {
        .mq_maxmsg = MQ_MAX_MESSAGES,
        .mq_msgsize = sizeof(MqMsg),
        .mq_flags = 0,
    };
    /* Destruction of the letterbox if it already exists */
    int8_t ret;
	ret = mq_unlink(queueName);
    if((ret == -1) && (errno != ENOENT)) // To avoid the case where the letterbox didn't exist
    {
        PERROR("Error to the return value of mq_unlink function\n");
    }
	// O_RDWR : Open the queue to both send and receive messages
	// S_IRUSR : Read Permission User, S_IRGRP : Read Permission Group Users
	myBal = mq_open(queueName, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP, &mqa);
    if(myBal == -1)
    {
        PERROR("Error to the return value of mq_open function\n");
    }
    myState = S_IDLE;

    PostmanBenno_createWritingPthread();
}

#ifndef _WRAP_POSTMANBENNO_CREATE_WRITING_PTHREAD
extern void PostmanBenno_createWritingPthread()
{
    int8_t ret;
    ret = pthread_create(&pthreadWrite, NULL, run, NULL);
    if(ret != 0)
    {
        PERROR("Error when creating the writing pthread\n");
        ret = 0;
    }

    ret = pthread_detach(pthreadWrite);
    if(ret != 0)
    {
        PERROR("Error when detaching the writing pthread\n");
    }
}
#endif

extern void PostmanBenno_start()
{
    MqMsg msg = {.data.event = E_CONNECTED};
	mqSend(&msg);
}

extern void PostmanBenno_free()
{
    /* Closing the letterbox */
    int8_t ret;
	ret = mq_close(myBal);
	if(ret == -1)
    {
        PERROR("Error to the return value of mq_close function\n");
    }

	/* Letterbox destruction */
	ret = mq_unlink(queueName);
	if(ret == -1)
    {
        PERROR("Error to the return value of mq_unlink function\n");
    }

    pthread_cancel(pthreadWrite);
}

extern void PostmanBenno_stop()
{
    MqMsg msg = {.data.event = E_STOP};
	mqSend(&msg);
}

extern void PostmanBenno_sendMsg(char * data)
{
    strncpy(sendBuffer, data, MAX_BUFFER_SIZE);
    MqMsg msg = {.data.event = E_SEND_MSG};
	mqSend(&msg);
}

static void notifyDisconnection()
{
    MqMsg msg = {.data.event = E_ERROR_COM};
	mqSend(&msg);
}

static void * readMsg(void * param)
{
    dataToReceive = true;

    while (dataToReceive)
    {
        int ret;
        ret = read(socketData, &receiveBuffer, sizeof(receiveBuffer));
        if (ret < 1)
        {
            dataToReceive = false;
            notifyDisconnection();
        }
        else
        {
            DispatcherBenno_setMsg(receiveBuffer);
        }

        memset(receiveBuffer, 0, sizeof(receiveBuffer)); // To empty the buffer
    }
    return NULL;
}

/*
 * Allow wrapping in testing mode.
 * Avoid symbol conflict in testing mode using a module name prefix.
 */
#ifndef _TESTING_MODE
static void postmanBenno_transitionFct(MqMsg msg) 
#else 
void postmanBenno_transitionFct(MqMsg msg); 
void __real_postmanBenno_transitionFct(MqMsg msg) 
#endif
{
    myTransition = &mySm[myState][msg.data.event];
    TRACE("\033[32m[PostmanBenno_Write] MAE, events management %s \033[0m\n",  eventGetName(msg.data.event));
    if (myTransition->destinationState != S_FORGET)
    {
        actionsTab[myTransition->action](); // Execution of the action
        TRACE("\033[32m[PostmanBenno_Write] MAE, action management %s \033[0m\n", actionGetName(myTransition->action));
        myState = myTransition->destinationState;
        TRACE("\033[32m[PostmanBenno_Write] MAE, going to state %s \033[0m\n", stateGetName(myState));
    }
    else
    {
        TRACE("\033[32m[PostmanBenno_Write] MAE, lost event %s  \033[0m\n", eventGetName(msg.data.event));
    }
}

static void * run(void * param)
{
    MqMsg msg;
    while (myState != S_DEATH)
    {
        mqReceive(&msg);
        if(msg.data.event == E_STOP)
        {
            myState = S_DEATH;
        }
        else
        {
            postmanBenno_transitionFct(msg);
        }
    }
    return NULL;
}

static void mqReceive(MqMsg * msg)
{
    int8_t ret;
    ret = mq_receive(myBal, msg->buffer, sizeof(MqMsg), NULL);
    if(ret != sizeof(MqMsg))
    {
        PERROR("Error to the return value of mq_receive function\n");
    }
}

static void mqSend(MqMsg * msg)
{
    int8_t ret;
    ret = mq_send(myBal, msg->buffer, sizeof(MqMsg), 0); // 0: priority of the message
    if(ret != 0)
    {
        PERROR("Error to the return value of mq_send function\n");
    }
}

static void actionNop()
{
  // Do nothing
}

static void actionStart()
{
    struct sockaddr_rc loc_addr, rem_addr; // RFCOMM socket address
    socklen_t opt = sizeof(rem_addr);

    // Allocate socket
    socketListener = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if(socketListener == -1)
    {
        PERROR("Error to the return value of socket function\n");
    }

    // Bind socket to port 1 of the first available
    // Local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = *BDADDR_ANY;
    loc_addr.rc_channel = (uint8_t)1;
    int8_t ret; // Allows to use one byte instead of 4
    ret = bind(socketListener, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    if(ret == -1)
    {
        PERROR("Error to the return value of bind function\n");
    }

    // Put socket into listening mode
    ret = listen(socketListener, MAX_PENDING_CONNECTIONS);
    if(ret == -1)
    {
        PERROR("Error to the return value of listen function\n");
    }

    // Accept one connection
    socketData = accept(socketListener, (struct sockaddr *)&rem_addr, &opt);
    if(socketData == -1)
    {
        PERROR("Error to the return value of accept function\n");
    }

    PostmanBenno_createReadingPthread();
}

#ifndef _WRAP_POSTMANBENNO_CREATE_READING_PTHREAD
extern void PostmanBenno_createReadingPthread()
{
    int8_t ret;
    ret = pthread_create(&pthreadRead, NULL, readMsg, NULL);
    if(ret != 0)
    {
        PERROR("Error when creating the reading pthread\n");
    }
}
#endif

static void actionWrite()
{
    int8_t ret;
    ret = write(socketData, sendBuffer, MAX_BUFFER_SIZE);
    TRACE("Données envoyées à Visio : %s\n", sendBuffer);
    if(ret == -1)
    {
        PERROR("Error to the return value of write function\n");
    }
}

static void actionNotifyDisconnection()
{
    Speaker_requestDataReceptionSignalKo();
    pthread_cancel(pthreadRead);
    
    int8_t ret;
    ret = close(socketData);
    if(ret == -1)
    {
        PERROR("Error when closing socketData\n");
        ret = 0;
    }
    ret = close(socketListener);
    if(ret == -1)
    {
        PERROR("Error when closing socketListener\n");
    }

    PostmanBenno_start();
}

static void actionStop()
{
    int8_t ret;
    ret = close(socketData);
    if(ret == -1)
    {
        PERROR("Error when closing socketData\n");
        ret = 0;
    }
    ret = close(socketListener);
    if(ret == -1)
    {
        PERROR("Error when closing socketListener\n");
    }
}
