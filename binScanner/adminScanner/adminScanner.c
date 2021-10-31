/**
 * @file adminScanner.c
 * @author Antoine Rivière
 * @brief adminScanner that set the state machine of the RFID sensor and 
 * manage the message queue for transitions.
 * @version 2.0
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#include <stdint.h>   // uint8_t, uint64_t
#include <stdio.h>    // printf
#include <sys/stat.h> /* Pour les constantes « mode » */
#include <mqueue.h>
#include <pthread.h>

#include "../../watchdog/watchdog.h"
#include "adminScanner.h"
#include "../scanner/scanner.h"
#include "../../utils.h"
#include "../rc522/rfidfct.h"
#include "../../../lib/bcm2835-1.56/bcm2835.h"
#include "./../../tourManager/tourManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  TYPEDEF & VARIABLES                                ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @enum AdminScanner state
 * @brief States of the state machine of AdminScanner class.
 * 
 */

typedef enum
{
    S_IDLE,       /**< Idle state*/
    S_ISSCANNING, /**< Scanning state*/
    S_WAITLIFT,   /**< Waiting lift for a new scanning state*/
    S_FORGET,     /**< Default state*/
    S_DEATH,      /**< State that happens to kill an active object*/
    S_NB          /**< State that gives the number of states*/
} State;

/**
 * @brief State_Name[] is an array that contains all states define in the 
 * type definition State.
 */
const char *const State_Name[] = {"S_IDLE", "S_ISSCANNING", "S_WAITLIFT",
                                  "S_FORGET", "S_DEATH", "S_NB"};

/**
 * @brief Function that return a string with the name of the i state in the array
 * 
 */
static const char *State_getName(int8_t i)
{
    return State_Name[i];
}

/**
 * @enum AdminScanner state
 * @brief Actions of the state machine
 * 
 */
typedef enum
{
    A_NOP,   /**< Action that does nothing*/
    A_SCAN,  /**< Action that start the reading of the RFID sensor*/
    A_NSCAN, /**< Action that stop the reading of the RFID sensor*/
    A_LIFT   /**< Action that simulate the bin lifting (by waiting 5 seconds 
                  before an other scan*/
} Action;

/**
 * @brief Action_Name[] is an array that contains all states define in the type 
 * definition Action.
 */
const char *const Action_Name[] = {"A_NOP", "A_SCAN", "A_NSCAN", "A_LIFT"};

/**
 * @brief Function that return a string with the name of the i action in the array
 * 
 */
static const char *Action_getName(int8_t i)
{
    return Action_Name[i];
}

/**
 * @brief Event_Name[] is an array that contains all states define in the type
 *  definition Events (Defined in adminScanner.h).
 */
const char *const Event_Name[] = {"E_START", "E_BINSCAN", "E_STOP",
                                  "E_EMPTYTIME", "E_NB"};

/**
 * @brief Events of the state machine
 * 
 */
typedef enum
{
    E_START,
    E_BINSCAN,
    E_STOP,
    E_EMPTYTIME,
    E_NB
} Events;

/**
 * @brief Function that return a string with the name of the i event in the array
 * 
 */
static const char *Event_getName(int8_t i)
{
    return Event_Name[i];
}

/**
 * @brief Definition of Transition type to create and manage all transition of the 
 * AdminScanner class's state machine.
 */
typedef struct
{
    State destinationState;
    Action action;
} Transition;

/**
 * @brief Definition of MqMsgs type to send and receive different events with 
 * function mqSend() and mqReceive()
 */
typedef struct
{
    Events event;
} MqMsgs;

static const char queueName[] = "/Bal";
static State myState = S_IDLE;
static pthread_t myThread;
static mqd_t myMQ;
static Transition *myTrans;
static IdBin idBin;
uint32_t delay = 5000000;
Watchdog *myWatchdog;

/**
 * @brief State machine of BinScanner package.
 * 
 */
static Transition mySm[S_NB][E_NB] =
    {
        [S_IDLE][E_START] = {S_ISSCANNING, A_SCAN},
        [S_ISSCANNING][E_STOP] = {S_IDLE, A_NSCAN},
        [S_ISSCANNING][E_START] = {S_ISSCANNING, A_SCAN},
        [S_ISSCANNING][E_BINSCAN] = {S_WAITLIFT, A_LIFT},
        [S_WAITLIFT][E_EMPTYTIME] = {S_ISSCANNING, A_SCAN},
        [S_WAITLIFT][E_STOP] = {S_IDLE, A_NSCAN}};

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  FUNCTIONS PROTOTYPES                               ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Start the search of a tag RFID to read: if there is a tag and it is 
 * known from tourData,it goes to the S_WAITLIFT state that simulate the lifting
 * of this bin. If there is no tag to read after 0.5 seconds,it goes to the S_IDLE 
 * state that wait and come again in the S_ISSCANNING state.
 */

/**
 * @brief Get the action from the state machine mySM and do differents action 
 * according to the action incoming.
 * 
 * @param anAction is the action from the state machine mySM.
 */
static void performAction(Action anAction);

/**
 * @brief Send the parameter msg in the message queue for being read when 
 * mqReceive is called.
 * 
 * @param msg is the type that contains the last event set by state machine.
 */
static void mqSend(MqMsgs msg);

/**
 * @brief Receive the last msg send with AdminScanner_mqSend that correspond to an Event.
 * 
 */
static MqMsgs mqReceive(void);

/**
 * @brief Action while A_NOP
 * 
 */
static void scanBin(void);

/**
 * @brief Action while A_NSCAN
 * 
 */
static void stopScanBin(void);

/**
 * @brief Action while A_LIFT 
 * 
 */
static void waitLiftBin(void);

/**
 * @brief Action while A_SCAN
 * 
 */
static void binScanned(void);

/**
 * @brief Routine of the thread. This while keep alive the state machine
 * 
 */
static void *run();

#ifndef _TESTING_MODE
static void adminScanner_transitionFct(MqMsgs msg);
#endif
/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   EXTERN FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

extern void AdminScanner_new()
{
    struct mq_attr attr =
        {
            .mq_flags = 0,
            .mq_maxmsg = 10,
            .mq_msgsize = sizeof(MqMsgs),
        };

    mq_unlink(queueName);

    myMQ = mq_open(queueName, O_CREAT | O_RDWR, 0777, &attr);

    myState = S_IDLE;
}

extern void AdminScanner_startBinScanner()
{
    // myWatchdog = Watchdog_construct(delay, stopScanBin);
    Scanner_initialise();
    pthread_create(&myThread, NULL, &run, NULL);
    pthread_detach(myThread);
}

extern void AdminScanner_stopBinScanner()
{

    // Watchdog_destroy(myWatchdog);
    Scanner_stop();
    pthread_cancel(myThread);
    // pthread_join(myThread, NULL);
}

extern void AdminScanner_free()
{
    /* Closing the letterbox */
    myState = S_FORGET;
    int8_t ret;
    ret = mq_close(myMQ);
    if (ret == -1)
    {
        PERROR("Error to the return value of mq_close function\n");
    }

    /* Letterbox destruction */
    ret = mq_unlink(queueName);
    if (ret == -1)
    {
        PERROR("Error to the return value of mq_unlink function\n");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   STATIC FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

static void scanBin()
{
    MqMsgs msg = {.event = E_START};
    mqSend(msg);
}

static void stopScanBin()
{
    MqMsgs msg = {.event = E_STOP};
    mqSend(msg);
}

static void waitLiftBin()
{
    MqMsgs msg = {.event = E_EMPTYTIME};
    mqSend(msg);
    sleep(5);
}
static void binScanned()
{
    MqMsgs msg = {.event = E_BINSCAN};
    mqSend(msg);
}

static void mqSend(MqMsgs msg)
{
    int8_t result = mq_send(myMQ, (const char *)&msg, sizeof(MqMsgs), 0);
    if (result != 0)
    {
        PERROR("Sending invalid value \n");
    }
}

static MqMsgs mqReceive()
{
    MqMsgs msg;
    int8_t result = mq_receive(myMQ, (char *)&msg, sizeof(MqMsgs), 0);
    return msg;
}

static void performAction(Action anAction)
{
    switch (anAction)
    {
    case A_NOP:
        if (myState == S_IDLE)
        {
            scanBin();
        }
        break;
    case A_SCAN:
        idBin = Scanner_askIdBin();
        Scanner_setIdBin(idBin);
        TourManager_setIdBinToTourManager(idBin);
        binScanned();
        break;
    case A_NSCAN:
        stopScanBin();
        break;
    case A_LIFT:
        // Watchdog_start(myWatchdog);
        waitLiftBin();
        break;
    default:
        break;
    }
}

#ifndef _TESTING_MODE
static void adminScanner_transitionFct(MqMsgs msg)
#else
void adminScanner_transitionFct(MqMsgs msg);
void __real_adminScanner_transitionFct(MqMsgs msg)
#endif
{
    Action myAction;
    myAction = mySm[myState][msg.event].action;

    performAction(myAction);
    myState = mySm[myState][msg.event].destinationState;
}

static void *run(Events anEVent)
{
    MqMsgs msg;
    Action myAction;

    scanBin();

    while (myState != S_DEATH)
    {
        msg = mqReceive();

        if (mySm[myState][msg.event].destinationState != S_FORGET)
        {
            adminScanner_transitionFct(msg);
        }
        else
        {
            // do nothing
        }
    }
    return NULL;
}