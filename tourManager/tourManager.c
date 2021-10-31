/**
 * @file tourManager.c
 * @author Damien Frissant, Léo Chauvin
 * @brief tourManager allows to receive the initial tour data from tourDirector and to send back the current tour data.
 * It also allows to start the boundary objects binScanner and scales.
 * @version 0.1
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
#include <unistd.h> // close
#include <sys/socket.h>
#include <json-c/json.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "tourManager.h"
#include "scales/scales.h"
#include "./../comBenno/proxyTourDirector/proxyTourDirector.h"
#include "./../comBenno/postmanBenno/postmanBenno.h"
#include "./../utils.h"
#include "../binScanner/scanner/scanner.h"
#include "../binScanner/adminScanner/adminScanner.h"

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  TYPEDEF & VARIABLES                                ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

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
 * @enum tourManager states
 * @brief States of the state machine. tourManager can have the following states: 
 * 
 */
typedef enum
{
    S_FORGET,              /**< Default state*/
    S_IDLE,                /**< Idle state*/
    S_WAITING_FOR_BIN,     /**< Waiting for bin state*/
    S_WAITING_FOR_WEIGHT,  /**< Waiting for weight state*/
    S_IS_LIFTING,          /**< Lifting state*/
    S_REST_FROM_IDLE,      /**< Rest state from Idle state*/
    S_REST_FROM_WF_BIN,    /**< Rest state from Waiting For Bin state*/
    S_REST_FROM_WF_WEIGHT, /**< Rest state from Waiting for weight state*/
    S_REST_FROM_LIFTING,   /**< Rest state from Lifting state*/
    S_ENDTOUR,             /**< End of Tour state*/
    S_DEATH,               /**< State to kill the active object*/
    STATE_NB,              /**< Allow us to have the number of states*/
} State;

/**
 * @brief Name of the states
 * 
 */
static const char *const stateName[] = {
    "S_FORGET",
    "S_IDLE",
    "S_WAITING_FOR_BIN",
    "S_WAITING_FOR_WEIGHT ",
    "S_IS_LIFTING",
    "S_REST_FROM_IDLE",
    "S_REST_FROM_WF_BIN",
    "S_REST_FROM_WF_WEIGHT",
    "S_REST_FROM_LIFTING",
    "S_ENDTOUR",
    "S_DEATH",
    "NB_STATE"};

/**
 * @brief Allow us to get the name of a state
 * 
 * @param i the 'id' of a state in "stateName"
 * @return const char* the name of the state
 */
static const char *stateGetName(int8_t i)
{
    return stateName[i];
}

/**
 * @enum TourManager action
 * @brief Actions of the state machine. TourManager can have the following actions: 
 * 
 */
typedef enum
{
    A_NOP,         /**<Action that does nothing*/
    A_ENDTOUR,     /**<Action when E_ENDTOUR event*/
    A_REST,        /**<Action when E_REST*/
    A_STOPREST,    /**<Action when E_STOPREST*/
    A_ENTERACTIVE, /**<Action when to enter into the Active super state*/
    A_IDLETOWFB,   /**<Action when we are passing from Idle state to Waiting for bin state*/
    A_SCANNING,    /**<Internal action of WaitingForBin*/
    A_ASKWEIGHT,   /**<Action when we are asking for the weight*/
    A_LIFT,        /**<Action to lift a bin*/
    A_COLLECTED,   /**<Action when a bin was collected*/
    ACTION_NB,     /**<Number of actions*/
} Action;

/**
 * @brief Name of the actions
 * 
 */
static const char *const actionName[] = {
    "A_NOP",
    "A_ENDTOUR",
    "A_REST",
    "A_STOPREST",
    "A_ENTERACTIVE",
    "A_IDLETOWFB",
    "A_SCANNING", //internal action of WaitingForBin
    "A_ASKWEIGHT",
    "A_LIFT",
    "A_COLLECTED",
    "ACTION_NB"};

/**
 * @brief Allow us to get the name of an action
 * 
 * @param i the 'id' of an action in "actionName"
 * @return const char* the name of the action
 */
static const char *actionGetName(int8_t i)
{
    return actionName[i];
}

/**
 * @enum TourManager events
 * @brief Events of the state machine. TourManager can have the following events:  
 * 
 */
typedef enum
{
    E_SET_TOURDATA, /**<Event to set the tour data*/
    E_SET_IDBIN,    /**<Event to set the id of a bin*/
    E_ASKWEIGHT,    /**<Event to ask the weight of a bin*/
    E_SET_WEIGHT,   /**<Event to set the weight of a bin*/
    E_TIME_OUT,     /**<Event when time out of emptying bin*/
    E_ACK_TOURDATA, /**<Event to acknowledge the tourData receive by Visio*/
    E_ENDTOUR,      /**<Event to end a tour*/
    E_REST,         /**<Event to begin a rest*/
    E_STOPREST,     /**<Event to stop a rest*/
    E_DEATH,        /**<Event to kill the active object*/
    EVENT_NB,       /**<Number of event*/
} Event;

/**
 * @brief Name of the events
 * 
 */
static const char *const eventName[] = {
    "E_SET_TOURDATA",
    "E_SET_IDBIN",
    "E_ASKWEIGHT",
    "E_SET_WEIGHT",
    "E_TIME_OUT",
    "E_ACK_TOURDATA",
    "E_ENDTOUR",
    "E_REST",
    "E_STOPREST",
    "E_DEATH",
    "EVENT_NB"};

/**
 * @brief Allow us to get the name of an event
 * 
 * @param i the 'id' of an event in "eventName"
 * @return const char* the name of the event
 */
static const char *eventGetName(int8_t i)
{
    return eventName[i];
}

typedef struct
{
    State destinationState;
    Action action;
} Transition;

static Transition *myTrans;

/**
 * @brief Definition of pointer's functions
 * 
 */
typedef void (*ActionPtr)();

static pthread_t myThread;
static mqd_t myBal;
static const char queueName[] = "/Balou";
static TourData myCurrentTourData;
static TourData myTourDataInProgress;
static BinData myCurrentBinData;
static IdBin myCurrentIdBin;
static BinState myCurrentBinState;
static Weight myCurrentWeight;

typedef struct
{
    Event event;
    IdBin aMqBinId;
    Weight aMqBinWeight;
    // generic definition to copy events' parameters
} MqMsgData;

/**
 * @brief Enable to send and receive messages without state convertion concern
 * 
 */
typedef union
{
    MqMsgData data;
    char mqBuffer[sizeof(MqMsgData)];
} MqMsg;

/**
 * @brief Current State, initialize at S_IDLE
 * 
 */
static State myState = S_IDLE;

/**
 * @brief State Machine of TourManager
 * 
 */
static Transition mySm[STATE_NB][EVENT_NB] = {
    //Transitions into Active super state
    [S_IDLE][E_SET_TOURDATA] = {S_WAITING_FOR_BIN, A_IDLETOWFB},
    [S_WAITING_FOR_BIN][E_SET_IDBIN] = {S_WAITING_FOR_BIN, A_SCANNING},
    [S_WAITING_FOR_BIN][E_ASKWEIGHT] = {S_WAITING_FOR_WEIGHT, A_ASKWEIGHT},
    [S_WAITING_FOR_WEIGHT][E_SET_WEIGHT] = {S_IS_LIFTING, A_LIFT},
    [S_IS_LIFTING][E_TIME_OUT] = {S_WAITING_FOR_BIN, A_COLLECTED},

    //From Active to EndTour
    [S_IDLE][E_ENDTOUR] = {S_ENDTOUR, A_ENDTOUR},
    [S_WAITING_FOR_BIN][E_ENDTOUR] = {S_ENDTOUR, A_ENDTOUR},
    [S_WAITING_FOR_WEIGHT][E_ENDTOUR] = {S_ENDTOUR, A_ENDTOUR},
    [S_IS_LIFTING][E_ENDTOUR] = {S_ENDTOUR, A_ENDTOUR},

    //From EndTour to Active
    [S_ENDTOUR][E_ACK_TOURDATA] = {S_IDLE, A_ENTERACTIVE},

    //From Active state to Rest state
    [S_IDLE][E_REST] = {S_REST_FROM_IDLE, A_REST},
    [S_WAITING_FOR_BIN][E_REST] = {S_REST_FROM_WF_BIN, A_REST},
    [S_WAITING_FOR_WEIGHT][E_REST] = {S_REST_FROM_WF_WEIGHT, A_REST},
    [S_IS_LIFTING][E_REST] = {S_REST_FROM_LIFTING, A_REST},

    //From Rest to Active
    [S_REST_FROM_IDLE][E_STOPREST] = {S_IDLE, A_STOPREST},
    [S_REST_FROM_WF_BIN][E_STOPREST] = {S_WAITING_FOR_BIN, A_STOPREST},
    [S_REST_FROM_WF_WEIGHT][E_STOPREST] = {S_WAITING_FOR_WEIGHT, A_STOPREST},
    [S_REST_FROM_LIFTING][E_STOPREST] = {S_IS_LIFTING, A_STOPREST},
};

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  FUNCTIONS PROTOTYPES                               ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Destroy the current tourData after f=data receive acknowledged
 * 
 */
static void destroyTourData(void);

/**
 * @brief Update the BinData during the tour after managing a bin \n
 * update the Date, the Time, the weight
 */
#ifndef _WRAP_UPDATEBD
static void updateBinData(void);
#else
void updateBinData(void);
#endif

/**
 * @brief Update the TourData during the turn after managing a bin \n
 * update the Date, the Time, the weight
 */
#ifndef _WRAP_UPDATETD
static void updateTourData(void);
#else
void updateTourData(void);
#endif

/**
 * @brief Check if the scanning bin is accepted or not \n
 * First the function is looking into tourData for the current idBin. Then, it is looking for the state of bin. After that the function is calling Speaker_requestBinStateSignal
 * @see Speaker_requestBinStateSignal(BinState)
 */
#ifndef _WRAP_CHECKIDBIN
static BinState checkIdBin(void);
#else
BinState checkIdBin(void);
#endif

/**
 * @brief Actions while A_ENTERACTIVE. From S_ENDTOUR to S_IDLE
 * 
 */
static void actionEnterActive(void);

/**
 * @brief No action => While Action A_NOP set
 * 
 */
static void actionNop(void);

/**
 * @brief Actions while A_ENDTOUR set
 * 
 */
static void actionEnd(void);

/**
 * @brief Actions while A_IDLETOWFB
 * 
 */
static void actionIdleToWfb(void);

/**
 * @brief Actions while A_SCANNING
 * BinState
 */
static void actionScanning(void);

/**
 * @brief Actions while A_COLLECTED
 * 
 */
static void actionCollected(void);

/**
 * @brief Action while A_REST
 * 
 */
static void actionRest(void);

/**
 * @brief Action while A_LIFT
 * 
 */
static void actionLift(void);

/**
 * @brief Action while A_STOPREST
 * 
 */
static void actionStopRest(void);

/**
 * @brief Function to call to generate the State Machine of tourManger as a puml file
 * 
 */
static void tourManagerMaeUml(void);

/**
 * @brief Routine of the thread. This while keep alive the state machine
 * 
 */
static void *run(void *aParam);

/**
 * @brief To receive a mqMessage
 * 
 * @param aMsg is a State
 */
static void mqReceive(MqMsg *aMsg);

/**
 * @brief To send a mqMessage
 * 
 * @param aMsg is a State
 */
static void mqSend(MqMsg *aMsg);

/**
 * @brief Transition from *run(...) function. Allow people to test the transition
 * 
 * @param msg, transmit an Event
 */
#ifndef _TESTING_MODE
static void tourManager_transitionFct(MqMsg msg);
#endif

/**
 * @brief Table of actions to realize => as a function pointers
 * 
 */
static const ActionPtr actionsTab[ACTION_NB] = {&actionNop, &actionEnd, &actionRest, &actionStopRest, &actionEnterActive, &actionIdleToWfb, &actionScanning, &Scales_askWeight, &actionLift, &actionCollected};

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   EXTERN FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

extern void TourManager_new()
{
    int8_t check;
    struct mq_attr attr = {
        .mq_maxmsg = MQ_MAX_MESSAGES,
        .mq_msgsize = sizeof(MqMsg),
        .mq_flags = 0,
    };

    check = mq_unlink(queueName);
    if ((check == -1) && (errno != ENOENT))
    {
        PERROR("Error when unlinking the BAL\n");
    }
    myBal = mq_open(queueName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &attr);
    if (myBal == -1)
    {
        PERROR("Error when opening the BAL\n");
    }
    myState = S_IDLE;
}

extern void TourManager_startBenno()
{
    Speaker_requestStartSignalOk();
    ClockMaker_setDefaultDateTime();
}

extern void TourManager_start()
{
    // State machine routine
    int8_t check;

    check = pthread_create(&myThread, NULL, &run, NULL);
    if (check != 0)
    {
        PERROR("Error to the return value of the thread cration\n");
    }
    TRACE("Launching\n");
    pthread_detach(myThread);
}

extern void TourManager_stop()
{
    MqMsg msg = {.data.event = E_DEATH};
    TRACE("Death Event\n");
    mqSend(&msg);
}

extern void TourManager_free()
{
    myState = S_FORGET;
    int8_t check;
    check = mq_close(myBal);
    if (check == -1)
    {
        PERROR("Error to the return value of mq_close function\n");
    }
    check = mq_unlink(queueName);
    if (check == -1)
    {
        PERROR("Error to the return value of mq_unlink function\n");
    }
    TRACE("Destruction\n");
}

extern void TourManager_setInitialTourData(TourData tourData)
{
    myCurrentTourData = tourData;
    MqMsg msg = {.data.event = E_SET_TOURDATA};
    TRACE("Set TourData\n");
    mqSend(&msg);
}

extern void TourManager_setIdBinToTourManager(IdBin idBin)
{
    myCurrentIdBin = idBin;
    MqMsg msg = {.data.event = E_SET_IDBIN, .data.aMqBinId = idBin};
    TRACE("Set IdBin\n");
    mqSend(&msg);
}

extern void TourManager_setWeight(Weight weight)
{
    myCurrentWeight = weight;
    MqMsg msg = {.data.event = E_SET_WEIGHT};
    TRACE("Set bin weight\n");
    mqSend(&msg);
}
extern void TourManager_timeOutEmptyTime()
{
    MqMsg msg = {.data.event = E_TIME_OUT};
    TRACE("Time out of the timer\n");
    mqSend(&msg);
}
extern void TourManager_ackTourData()
{
    MqMsg msg = {.data.event = E_ACK_TOURDATA};
    TRACE("Enter into active, previously in S_ENDTOUR\n");
    mqSend(&msg);
}

extern void TourManager_signalEndTour()
{
    MqMsg msg = {.data.event = E_ENDTOUR};
    TRACE("End the current tour\n");
    mqSend(&msg);
}

extern void TourManager_requestToAskWeight()
{
    MqMsg msg = {.data.event = E_ASKWEIGHT};
    TRACE("Requesto to ask bin weight\n");
    mqSend(&msg);
}

extern void TourManager_startRest()
{
    MqMsg msg = {.data.event = E_REST};
    TRACE("Start Rest\n");
    mqSend(&msg);
}

extern void TourManager_stopRest()
{
    MqMsg msg = {.data.event = E_STOPREST};
    TRACE("Stop Rest\n");
    mqSend(&msg);
}

extern void TourManager_requestToStopTools()
{
    #ifndef _TESTING_MODE
    exit(EXIT_SUCCESS);
    #endif
    AdminScanner_stopBinScanner();
    Scales_free();
    PostmanBenno_stop();
    PostmanBenno_free();
    TourManager_stop();
}

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   STATIC FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _TESTING_MODE
static void tourManager_transitionFct(MqMsg msg)
#else
void tourManager_transitionFct(MqMsg msg);
void __real_tourManager_transitionFct(MqMsg msg)
#endif
{
    myTrans = &mySm[myState][msg.data.event];

    TRACE("\033[33m MAE, events management %s \033[0m\n", eventGetName(msg.data.event));

    if (myTrans->destinationState != S_FORGET)
    {
        actionsTab[myTrans->action](); //execution of the action
        TRACE("\033[33m MAE, actions management %s \033[0m\n", actionGetName(myTrans->action));
        myState = myTrans->destinationState;
        TRACE("\033[33m MAE, going to state %s \033[0m\n", stateGetName(myState));
    }
    else
    {
        TRACE("\033[33mMAE, lost event %s  \033[0m\n", eventGetName(msg.data.event));
    }
}

static void *run(void *aParam)
{
    MqMsg msg;
    while (myState != S_FORGET)
    {
        mqReceive(&msg);
        if (msg.data.event == E_DEATH)
        {
            myState = S_FORGET;
            TourManager_free();
        }
        else
        {
            tourManager_transitionFct(msg);
        }
    }
    return NULL;
}

static void mqReceive(MqMsg *aMsg)
{
    int8_t check;
    check = mq_receive(myBal, aMsg->mqBuffer, sizeof(MqMsg), NULL);
    if (check == -1)
    {
        PERROR("Error to the return value of mq_receive function\n");
    }
}

static void mqSend(MqMsg *aMsg)
{
    int8_t check;
    check = mq_send(myBal, aMsg->mqBuffer, sizeof(MqMsg), 0);
    if (check != 0)
    {
        PERROR("Error to the return value of mq_send function\n");
    }
}

static void actionNop()
{
    TRACE("[TourManager] Action NOP \n");
}

static void actionEnd()
{
    ProxyTourDirector_setCurrentTourData(myTourDataInProgress);
    AdminScanner_stopBinScanner();
    AdminScanner_free();
    Scales_free();
}

static void actionEnterActive()
{
    destroyTourData(); //in case the previous one was not destroy
}

static void actionIdleToWfb(void)
{
    myTourDataInProgress = json_object_new_array();
    Speaker_requestDataReceptionSignalOk();
    AdminScanner_new();
    Scanner_initialise();
    AdminScanner_startBinScanner();
    Scales_new();
}

static void actionScanning()
{
    myCurrentBinState = checkIdBin();
    Speaker_requestBinStateSignal(myCurrentBinState);
}

static void actionCollected()
{
    updateBinData();
    updateTourData();
    ProxyTourDirector_setCurrentBinData(myCurrentBinData);
}

static void actionRest()
{
    Speaker_requestRestSignal();
    AdminScanner_stopBinScanner();
    AdminScanner_free();
}

static void actionStopRest()
{
    Speaker_requestEndOfRestSignal();
    AdminScanner_new();
    Scanner_initialise();
    AdminScanner_startBinScanner();
}

static void actionLift()
{
    Speaker_requestToEmptyBin();
}

#ifndef _WRAP_CHECKIDBIN
static BinState checkIdBin(void)
{
    struct json_object *binListObj;
    struct json_object *binListObjIdBin;
    struct json_object *binListObjIdBinState;
    uint8_t arrayLen;
    BinState binStateToCheck = BINSTATE_UNKNOWN;

    arrayLen = json_object_array_length(myCurrentTourData);

    for (int i = 0; i < arrayLen; i++)
    {
        // get the i-th object of myCurrentTourData
        binListObj = json_object_array_get_idx(myCurrentTourData, i);
        // get the name attribute in the i-th object, print out the Id of the bin, then the bin State
        json_object_object_get_ex(binListObj, "IdBin", &binListObjIdBin);
        const char *idBinJson = json_object_get_string(binListObjIdBin);

        //Check the idBin
        if (strcmp(myCurrentIdBin.idBin, idBinJson) == 0)
        {
            //Check binState
            json_object_object_get_ex(binListObj, "BinState", &binListObjIdBinState);
            const char *binStateJson = json_object_get_string(binListObjIdBinState);
            if (strcmp(binStateJson, "BINSTATE_ACCEPTED") == 0)
            {
                binStateToCheck = BINSTATE_ACCEPTED;
            }
            else
            {
                binStateToCheck = BINSTATE_REFUSED;
            }
            break; //Allows to stop the loop when the id is found
        }
        else
        {
            binStateToCheck = BINSTATE_UNKNOWN; //File browsed but the Id was not find
        }
    }
    return binStateToCheck;
}
#endif

#ifndef _WRAP_UPDATEBD
static void updateBinData(void)
{
    bool binFound;
    binFound = false;
    struct json_object *binData = json_object_new_object();
    uint8_t arrayLen;
    arrayLen = json_object_array_length(myCurrentTourData);
    for (int i = 0; i < arrayLen; i++)
    {
        struct json_object *binToCheck = json_object_array_get_idx(myCurrentTourData, i);
        struct json_object *idBinToCheck = json_object_new_object();
        json_object_object_get_ex(binToCheck, "IdBin", &idBinToCheck);
        const char *valueOfId = json_object_get_string(idBinToCheck);
        if (strcmp(myCurrentIdBin.idBin, valueOfId) == 0)
        {
            binData = binToCheck;
            binFound = true;
        }
    }
    if (false == binFound) // BIN UNKNOWN
    {
        json_object_object_add(binData, "IdBin", json_object_new_string(myCurrentIdBin.idBin));
        struct json_object *defaultPosition = json_object_new_object();
        json_object_object_add(binData, "Position", defaultPosition);
        json_object_object_add(defaultPosition, "lat", json_object_new_string(""));
        json_object_object_add(defaultPosition, "lng", json_object_new_string(""));
        json_object_object_add(binData, "BinState", json_object_new_string("BINSTATE_UNKNOWN"));
    }

    DateTime dateTime = ClockMaker_getCurrentDateTime();
    char weightToChar[4];
    sprintf(weightToChar, "%d", myCurrentWeight);
    json_object_object_add(binData, "DateTime", json_object_new_string(dateTime.data));
    json_object_object_add(binData, "Weight", json_object_new_string(weightToChar));

    myCurrentBinData = binData;
}
#endif

#ifndef _WRAP_UPDATETD
static void updateTourData(void)
{
    json_object_array_add(myTourDataInProgress, myCurrentBinData);
}
#endif

static void destroyTourData(void)
{
    myCurrentTourData = NULL;
}
