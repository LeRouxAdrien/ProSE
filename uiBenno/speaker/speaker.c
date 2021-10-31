/**
 * @file speaker.c
 * @author Adrien Le Roux
 * @brief speaker interact with Eboueur thanks to leds, but also with lifting and force button.
 * @version 0.1
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */
#include <mqueue.h>
#include <stdint.h>  // uint8_t, uint64_t
#include <stdio.h>   // printf
#include <stdbool.h> // bool
#include <pthread.h> // threads library
#include <errno.h>
#include "speaker.h"
#include "./../leds/leds.h"
#include "../../watchdog/watchdog.h"
#include "../forcingButton/forcingButton.h"
#include "../liftingButton/liftingButton.h"
#include "../stopButton/stopButton.h"
#include "./../../utils.h"
#include "../../tourManager/tourManager.h"

#define MQ_MAX_MESSAGES 10

//////////////////////////////////////////////
//////////  DEFINE TYPES AND STRUCT  /////////
//////////////////////////////////////////////
/**
 * @brief States of the state machine, the first one must be S_FORGET 
 * 
 */
typedef enum
{
    S_FORGET = 0,            /**<Default state>*/
    S_START,                 /**<Start state>*/
    S_CONNECTED,             /**<State when Visio and Benno are connected>*/
    S_WAITINGFORSIGNAL,      /**<State waiting for a binstate>*/
    S_BINEVAL,               /**<State Evaluate the bin state>*/
    S_BIN_REFUSED,           /**<State bin refused>*/
    S_BIN_UNKNOWN,           /**<State bin unknown>*/
    S_BIN_ACCEPTED,          /**<State bin accepted>*/
    S_BIN_WEIGHING,          /**<State bin is weighing>*/
    S_BINEMTYING,            /**<State bin is emptying>*/
    S_REST_FROM_START,       /**<State rest from start>*/
    S_REST_FROM_CONNECTED,   /**<State rest from connected>*/
    S_REST_FROM_WAITINGFS,   /**<State rest from waiting for signal>*/
    S_REST_FROM_BINEVAL,     /**<State rest from bin evaluate>*/
    S_REST_FROM_REFUSED,     /**<State rest from bin refused>*/
    S_REST_FROM_UNKNOWN,     /**<State rest from bin unkwnow>*/
    S_REST_FROM_ACCEPTED,    /**<State rest from bin accepted>*/
    S_REST_FROM_WEIGHING,    /**<State rest from bin is weighing>*/
    S_REST_FROM_BINEMPTYING, /**<State rest from bin is emptying>*/
    STATE_NB,
} State;

const char *const stateName1[] = {"S_FORGET", "S_START",
                                  "S_CONNECTED", "S_WAITINGFORSIGNAL", "S_BINEVAL", "S_BIN_REFUSED",
                                  "S_BIN_UNKNOWN", "S_BIN_ACCEPTED", "S_BIN_WEIGHING", "S_BINEMTYING",
                                  "S_REST_FROM_START", "S_REST_FROM_CONNECTED", "S_REST_FROM_WAITINGFS", "S_REST_FROM_BINEVAL",
                                  "S_REST_FROM_REFUSED", "S_REST_FROM_ACCEPTED", "S_REST_FROM_WEIGHING", "S_REST_FROM_UNKNOWN", "S_REST_FROM_BINEMPTYING", "STATE_NB"};
static const char *stateGetName(int8_t i)
{
    return stateName1[i];
}
/**
 * @brief Actions of the state machine, the first one must be A_NOP
 * 
 */
typedef enum
{
    A_NOP = 0,                    /**<Action to do nothing>*/
    A_EXIT_ACTIVE,                /**<action to exit from the active mode>*/
    A_ENTERACTIVE,                /**<action enter in active state>*/
    A_STARTTOCONNECTED,           /**<action enter from start to connected>*/
    A_CONTOWFS,                   /**<action enter from connected to waiting for signal>*/
    A_WFSTOBINEVAL,               /**<action enter from waiting for signal to BinStateEvaluate>*/
    A_BINEVAL_BINREFUSED,         /**<action enter from BinStateEvaluate to Refused>*/
    A_BINEVAL_BINACCEPTED,        /**<action enter from BinStateEvaluate to Accepted>*/
    A_BINEVAL_BINUNKNOWN,         /**<action enter from BinStateEvaluate to Unknown>*/
    A_BINREFUSED_TO_BINACCEPTED,  /**<action enter from Refused to Accepted>*/
    A_BINUNKNOWN_TO_BINACCEPTED,  /**<action enter from Unknown to Accepted>*/
    A_BINACCEPTED_TO_BINWEIGHING, /**<action enter from Accepted to BinWeighing>*/
    A_BINWEIGHING_TO_BINEMPTYING, /**<action enter from BinWeighing to BinEmptying>*/
    A_BINEMPTYINGTOWFS,           /**<action enter from BinEmptying to Waiting for Signal>*/
    A_REST_TO_STARTACTIVE,        /**<action enter from Rest to Start Active>*/
    A_REST_TO_CONNECTED,          /**<action enter from Rest To Connected>*/
    A_REST_TO_WFS,                /**<action enter from Rest to Waiting for Signal>*/
    A_REST_TO_BINEVAL,            /**<action enter from Rest to Bin Evaluate>*/
    A_REST_TO_BINREFUSED,         /**<action enter from Rest to Refused>*/
    A_REST_TO_BINACCEPTED,        /**<action enter from Rest to Accepted>*/
    A_REST_TO_BINUNKNOWN,         /**<action enter from Rest to Unknown>*/
    A_REST_TO_BINWEIGHING,        /**<action enter from Rest to Bin Weighing>*/
    A_REST_TO_BINEMPTYING,        /**<action enter from Rest to Bin Emptying>*/
    A_START_TO_REST,              /**<action enter from Start to Rest>*/
    A_CONNECTED_TO_REST,          /**<action enter from Connected to Rest>*/
    A_WFS_TO_REST,                /**<action enter from Waiting for Signal to Rest>*/
    A_BINEVAL_TO_REST,            /**<action enter from Bin Evaluate to Rest>*/
    A_BINREFUSED_TO_REST,         /**<action enter from Refused to Rest>*/
    A_BINUNKNOWN_TO_REST,         /**<action enter from Unknown to Rest>*/
    A_BINACCEPTED_TO_REST,        /**<action enter from Accepted to Rest>*/
    A_BINWEIGHING_TO_REST,        /**<action enter from Bin Weighing to Rest>*/
    A_BINEMPTYING_TO_REST,        /**<action enter from Bin Emptying to Rest>*/
    ACTION_NB,                    /**<    >*/
} Action;
const char *const actionName1[] = {"A_NOP", "A_EXIT_ACTIVE", "A_ENTERACTIVE",
                                   "A_STARTTOCONNECTED", "A_CONTOWFS ", "A_WFSTOBINEVAL", "A_BINEVAL_BINREFUSED",
                                   "A_BINEVAL_BINACCEPTED", "A_BINEVAL_BINUNKNOWN", "A_BINREFUSED_TO_BINACCEPTED",
                                   "A_BINWEIGHING_TO_BINEMPTYING", "A_BINACCEPTED_TO_BINWEIGHING",
                                   "A_BINEMPTYINGTOWFS", "A_REST_TO_STARTACTIVE", "A_REST_TO_CONNECTED",
                                   "A_REST_TO_WFS", "A_REST_TO_BINEVAL", "A_REST_TO_BINREFUSED",
                                   "A_REST_TO_BINACCEPTED", "A_REST_TO_BINUNKNOWN", "A_REST_TO_BINWEIGHING",
                                   "A_REST_TO_BINEMPTYING", "A_START_TO_REST", "A_CONNECTED_TO_REST",
                                   "A_WFS_TO_REST", "A_BINEVAL_TO_REST", "A_BINREFUSED_TO_REST", "A_BINUNKNOWN_TO_REST",
                                   "A_BINACCEPTED_TO_REST", "A_BINWEIGHING_TO_REST", "A_BINEMPTYING_TO_REST", "ACTION_NB"};
static const char *actionGetName(int8_t i)
{
    return actionName1[i];
}
/**
 * @brief Events of the state machine
 * 
 */
typedef enum
{
    E_STOP_BENNO = 0,         /**<Evènement Speaker_stopBenno>*/
    E_RE_CONNEXION_SIOK,      /**<Evènement Speaker_requestConnexionSignalOk>*/
    E_RE_DATA_CONNEXIONOK,    /**<Evènement Speaker_requestDataReceptionSignalOk>*/
    E_RE_BIN_STATE,           /**<Evènement Speaker_requestBinState>*/
    E_EVAL_BINSTATE_REFUSED,  /**<Evènement bin refused>*/
    E_EVAL_BINSTATE_ACCEPTED, /**<Evènement bin accepted>*/
    E_EVAL_BINSTATE_UNKNOWN,  /**<Evènement bin unknown>*/
    E_RE_EMPTY_BIN,           /**<Evènement Speaker_requestEmptyBin>*/
    E_FORCE_LIF_BIN,          /**<Evènement Speaker_forceLiftBin>*/
    E_LIFT_BIN,               /**<Evènement Speaker_LiftBin>*/
    E_TO_EM_TIME,             /**<Evènement timeOutEmptyTime>*/
    E_RE_ENDOFREST,           /**<Evènement Speaker_requestEndOfRest>*/
    E_RE_REST,                /**<Evènement Speaker_requestRest>*/
    EVENT_NB,                 /**<    >*/
} Event;
const char *const eventName1[] = {"E_STOP_BENNO", "E_RE_CONNEXION_SIOK",
                                  "E_RE_DATA_CONNEXIONOK", "E_RE_BIN_STATE", "E_EVAL_BINSTATE_REFUSED", "E_EVAL_BINSTATE_ACCEPTED",
                                  "E_EVAL_BINSTATE_UNKNOWN", "E_RE_EMPTY_BIN", "E_FORCE_LIF_BIN", "E_LIFT_BIN",
                                  "E_TO_EM_TIME", "E_RE_ENDOFREST",
                                  "E_RE_REST", "EVENT_NB"};
static const char *eventGetName(int8_t i)
{
    return eventName1[i];
}
/**
 * @brief Routine of the thread. This while keep alive the state machine
 * 
 */
static void *run(void *aParam);

typedef struct
{
    State destinationState;
    Action action;
} Transition;

static Transition *myTrans;

typedef struct
{
    Event event;
    BinState msqBinState;
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
 * @brief Definition of pointer's functions
 * 
 */
typedef void (*ActionPtr)();

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

#ifndef _TESTING_MODE
/**
 * @brief Transition from *run(...) function. Allow people to test the transition
 * 
 * @param msg, transmit an Event
 */
static void transitionFct1(MqMsg msg);
#endif 
/**
 * @brief State Machine of TourManager
 *
 */
static Transition mySm[STATE_NB][EVENT_NB] = {

    //Transitions into Active super state
    [S_START][E_RE_CONNEXION_SIOK] = {S_CONNECTED, A_STARTTOCONNECTED},
    [S_START][E_RE_REST] = {S_REST_FROM_START, A_START_TO_REST},
    [S_CONNECTED][E_RE_DATA_CONNEXIONOK] = {S_WAITINGFORSIGNAL, A_CONTOWFS},
    [S_CONNECTED][E_RE_REST] = {S_REST_FROM_CONNECTED, A_CONNECTED_TO_REST},
    [S_WAITINGFORSIGNAL][E_RE_BIN_STATE] = {S_BINEVAL, A_WFSTOBINEVAL},
    [S_WAITINGFORSIGNAL][E_RE_REST] = {S_REST_FROM_WAITINGFS, A_WFS_TO_REST},
    [S_BINEVAL][E_EVAL_BINSTATE_REFUSED] = {S_BIN_REFUSED, A_BINEVAL_BINREFUSED},
    [S_BINEVAL][E_EVAL_BINSTATE_UNKNOWN] = {S_BIN_UNKNOWN, A_BINEVAL_BINUNKNOWN},
    [S_BINEVAL][E_EVAL_BINSTATE_ACCEPTED] = {S_BIN_ACCEPTED, A_BINEVAL_BINACCEPTED},
    [S_BINEVAL][E_RE_REST] = {S_REST_FROM_BINEVAL, A_BINEVAL_TO_REST},
    [S_BIN_REFUSED][E_FORCE_LIF_BIN] = {S_BIN_ACCEPTED, A_BINREFUSED_TO_BINACCEPTED},
    [S_BIN_REFUSED][E_RE_REST] = {S_REST_FROM_REFUSED, A_BINREFUSED_TO_REST},
    [S_BIN_UNKNOWN][E_FORCE_LIF_BIN] = {S_BIN_ACCEPTED, A_BINUNKNOWN_TO_BINACCEPTED},
    [S_BIN_UNKNOWN][E_RE_REST] = {S_REST_FROM_UNKNOWN, A_BINUNKNOWN_TO_REST},
    [S_BIN_ACCEPTED][E_LIFT_BIN] = {S_BIN_WEIGHING, A_BINACCEPTED_TO_BINWEIGHING},
    [S_BIN_ACCEPTED][E_RE_REST] = {S_REST_FROM_ACCEPTED, A_BINACCEPTED_TO_REST},
    [S_BIN_WEIGHING][E_RE_EMPTY_BIN] = {S_BINEMTYING, A_BINWEIGHING_TO_BINEMPTYING},
    [S_BIN_WEIGHING][E_RE_REST] = {S_REST_FROM_WEIGHING, A_BINWEIGHING_TO_REST},
    [S_BINEMTYING][E_TO_EM_TIME] = {S_WAITINGFORSIGNAL, A_BINEMPTYINGTOWFS},
    [S_BINEMTYING][E_RE_REST] = {S_REST_FROM_BINEMPTYING, A_BINEMPTYING_TO_REST},
    //From Active to Rest
    [S_REST_FROM_START][E_RE_ENDOFREST] = {S_START, A_REST_TO_STARTACTIVE},
    [S_REST_FROM_CONNECTED][E_RE_ENDOFREST] = {S_CONNECTED, A_REST_TO_CONNECTED},
    [S_REST_FROM_WAITINGFS][E_RE_ENDOFREST] = {S_WAITINGFORSIGNAL, A_REST_TO_WFS},
    [S_REST_FROM_BINEVAL][E_RE_ENDOFREST] = {S_BINEVAL, A_REST_TO_BINEVAL},
    [S_REST_FROM_REFUSED][E_RE_ENDOFREST] = {S_BIN_REFUSED, A_REST_TO_BINREFUSED},
    [S_REST_FROM_UNKNOWN][E_RE_ENDOFREST] = {S_BIN_UNKNOWN, A_REST_TO_BINUNKNOWN},
    [S_REST_FROM_ACCEPTED][E_RE_ENDOFREST] = {S_BIN_ACCEPTED, A_REST_TO_BINACCEPTED},
    [S_REST_FROM_WEIGHING][E_RE_ENDOFREST] = {S_BIN_WEIGHING, A_REST_TO_BINWEIGHING},
    [S_REST_FROM_BINEMPTYING][E_RE_ENDOFREST] = {S_BINEMTYING, A_REST_TO_BINEMPTYING},
    //From Active to EndTour
    [S_START][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_CONNECTED][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_WAITINGFORSIGNAL][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_BINEVAL][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_BIN_REFUSED][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_BIN_UNKNOWN][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_BIN_ACCEPTED][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_BIN_WEIGHING][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
    [S_BINEMTYING][E_STOP_BENNO] = {S_FORGET, A_EXIT_ACTIVE},
};

static bool inhibitor = true;
static mqd_t myBal;
static const char queueName[] = "/Balou";
static BinState myCurrentBinState;
static Led *leds;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static pthread_t myThread;
static State myState = S_START;
static Watchdog *wat;
//////////////////////////////////////////////
//////////   DEFINE ALL FUNCTIONS   //////////
//////////////////////////////////////////////

/**
 * @brief evaluate the binState
 * 
 */
static void evaluateBinState(BinState binState);

/**
 * @brief indicate that the raspberry power is ok by a blink of the white led with 0,5 period off and 0.5 on during 3 secondes
 * 
 */
static void indicateStartOk();

/**
 * @brief indicate that the connexion between the raspberry and android table has been succesful by a continue white lighting.
 * 
 */
static void indicateConnectionOk();

/**
 * @brief indicate that connexion has been established between raspberry and android table by a blink of the orange led with 0,5 period off and 0.5 on during 3 secondes
 * 
 */
static void indicateDataReceptionOk();

/**
 * @brief indicate that there is a connexion problem by a blink of the white led with 0,5 period off and 0.5 on during 3 secondes
 * 
 */
static void indicateDataReceptionKo();

/**
 * @brief indicate if the bin has been refused, unknown and accepted after being scanning. A red continue lighting during 3 secondes
 * if the bin is refused, green continue lighting during 3 secondes if the bin is accepted and an orange continue lighting during 3 secondes 
 * if the bin is not recognized. 
 * 
 * @param binState 
 */
static void indicateBinState(BinState binState);

/**
 * @brief indicate if the starting of a rest with a blinking of the white led.  
 * 
 */
static void indicateRest();

/**
 * @brief indicate the end of the rest by a white led continue blinking   
 * 
 */
static void indicateEndOfRest();

/**
 * @brief empty the bin
 * 
 */
static void emptyBin();

/**
 * @brief Send a text message on the message queue when the empty time is done
 * 
 */
static void timeOutEmptyTime();

/**
 * @brief set the time for the timer EMPTY_TIME.
 * 
 */
static void setTimeEmptyTime(Watchdog *timer);

/**
 * @brief reset the timer
 * 
 */
static void reset();

/**
 * @brief deactivate the inhibitor
 * 
 */
static void deactivateBinInhibitor();

/**
 * @brief activate the inhibitor
 * 
 */
static void activateBinInhibitor();

/**
 * @brief initialize all gpios and assigned pins to a specific composant and create threads
 * 
 */
static void init();

/**
 * @brief initialize and start the MàE of speaker
 * 
 */
static void startBenno();

/**
 * @brief action Nop
 * 
 */
static void actionNop();

/**
 * @brief action to exit from the active mode
 * 
 */
static void actionExit();
/**
 * @brief action enter in active state
 * 
 */
static void actionEnterActive();

/**
 * @brief action enter from start to connected
 * 
 */
static void actionStartToConnected();

/**
 * @brief action enter from connected to waiting for signal
 * 
 */
static void actionConToWfs();

/**
 * @brief action enter from waiting for signal to BinStateEvaluate
 * 
 */
static void actionWfsToBinEval();

/**
 * @brief action enter from BinStateEvaluate to Refused
 * 
 */
static void actionBinEvalRefused();

/**
 * @brief action enter from BinStateEvaluate to Accepted
 * 
 */
static void actionBinEvalAccepted();

/**
 * @brief action enter from BinStateEvaluate to Unknown
 * 
 */
static void actionBinEvalUnknown();

/**
 * @brief action enter from Refused to Accepted
 * 
 */
static void actionBinRefusedToBinAccepted();

/**
 * @brief action enter from Unknown to Accepted
 * 
 */
static void actionBinUnknownToBinAccepted();

/**
 * @brief action enter from Accepted to BinWeighing
 * 
 */
static void actionBinAcceptedToBinWeighing();

/**
 * @brief action enter from BinWeighing to BinEmptying
 * 
 */
static void actionBinWeighingToBinEmptying();

/**
 * @brief action enter from BinEmptying to Waiting for Signal
 * 
 */
static void actionBinEmptyingToWfs();
/**
 * @brief action enter from Rest to Start Active
 * 
 */
static void actionRestToStartActive();
/**
 * @brief action enter from Rest To Refused
 * 
 */
static void actionRestToConnected();

/**
 * @brief action enter from Rest To Waiting for Signal
 * 
 */
static void actionRestToWfs();

/**
 * @brief action enter from Rest to BinEvaluate
 * 
 */
static void actionRestToBinEval();

/**
 * @brief action enter from Rest to Refused
 * 
 */
static void actionRestToBinRefused();

/**
 * @brief action enter from Rest to Accepted
 * 
 */
static void actionRestToBinAccepted();

/**
 * @brief action enter from Rest to Unknown
 * 
 */
static void actionRestToBinUnknown();

/**
 * @brief action enter from Rest to BinWeighing
 * 
 */

static void actionRestToBinWeighing();

/**
 * @brief action enter from Rest to BinEmptying
 * 
 */
static void actionRestToBinEmptying();

/**
 * @brief action enter from Start to Rest
 * 
 */
static void actionStartToRest();

/**
 * @brief action enter from Connected to Rest
 * 
 */
static void actionConnectedToRest();

/**
 * @brief action enter from BinEvaluate to Rest
 * 
 */
static void actionBinEvalToRest();

/**
 * @brief action enter from Refused to Rest
 * 
 */
static void actionBinRefusedToRest();

/**
 * @brief action enter from Unknown to Rest
 * 
 */
static void actionBinUnknownToRest();

/**
 * @brief action enter from Accepted to Rest
 * 
 */
static void actionBinAcceptedToRest();

/**
 * @brief action enter from BinWeighing to Rest
 * 
 */
static void actionBinWeighingToRest();

/**
 * @brief action enter from BinEmptying to Rest
 * 
 */
static void actionBinEmptyingToRest();

/**
 * @brief This is the function which call the TourManager time out function when EMPTY_TIME has been passed. 
 * 
 * @param this is a watchdog parameter 
 */
static void Speaker_timeOut(Watchdog *this);

/**
 * @brief turn into the coding state machine into a puml file
 * 
 */
static void tourManagerMaeUml();

/**
 * @brief Table of actions to realize
 * 
 */
static const ActionPtr actionsTab[ACTION_NB] =
    {&actionNop, &actionExit,
     &actionEnterActive, &actionStartToConnected, &actionConToWfs,
     &actionWfsToBinEval, &actionBinEvalRefused, &actionBinEvalAccepted, &actionBinEvalUnknown,
     &actionBinRefusedToBinAccepted, &actionBinUnknownToBinAccepted, &actionBinAcceptedToBinWeighing,
     &actionBinWeighingToBinEmptying, &actionBinEmptyingToWfs, &actionRestToStartActive,
     &actionRestToConnected, &actionRestToWfs, &actionRestToBinEval,
     &actionRestToBinRefused, &actionRestToBinAccepted, &actionRestToBinUnknown,
     &actionRestToBinWeighing, &actionRestToBinEmptying,
     &actionStartToRest, &actionConnectedToRest, &actionBinEvalToRest,
     &actionBinRefusedToRest, &actionBinUnknownToRest, &actionBinAcceptedToRest,
     &actionBinWeighingToRest, &actionBinEmptyingToRest};

//////////////////////////////////////////////
//////////     MAE FUNCTIONS      ////////////
//////////////////////////////////////////////

#ifndef _TESTING_MODE
static void transitionFct1(MqMsg msg)
#else
void transitionFct1(MqMsg msg);
void __real_transitionFct1(MqMsg msg)
#endif 
{
    myTrans = &mySm[myState][msg.data.event];

    if (myTrans->destinationState != S_FORGET)
    {
        actionsTab[myTrans->action](); //execution of the action
        myState = myTrans->destinationState;
        TRACE("\033[35m MAE, going to state %s \033[0m \n", stateGetName(myState));
    }
    else
    {
        // Do nothing
    }
}

static void *run(void *aParam)
{
    MqMsg msg;
    while (myState != S_FORGET)
    {
        mqReceive(&msg);
        if (msg.data.event == E_STOP_BENNO)
        {
            myState = S_FORGET;
        }
        else
        {
            transitionFct1(msg);
        }
    }
    return NULL;
}
static void mqReceive(MqMsg *aMsg)
{
    int8_t check;
    check = mq_receive(myBal, (char *)aMsg, sizeof(*aMsg), NULL);
    if (check == -1)
    {
        PERROR("Error when receive message\n");
    }
}

static void mqSend(MqMsg *aMsg)
{
    int8_t check;
    check = mq_send(myBal, (const char *)aMsg->mqBuffer, sizeof(MqMsg), 0);
    if (check != 0)
    {
        PERROR("Error when sending the message \n");
    }
}

static void actionNop()
{
}

static void actionExit()
{
    TourManager_requestToStopTools();
}

static void actionEnterActive()
{
    indicateStartOk();
}

static void actionStartToConnected()
{
    indicateConnectionOk();
}

static void actionConToWfs()
{
    indicateDataReceptionOk();
}

static void actionWfsToBinEval()
{
    evaluateBinState(myCurrentBinState);
}

static void actionBinEvalRefused()
{
    indicateBinState(myCurrentBinState);
}

static void actionBinEvalAccepted()
{
    indicateBinState(myCurrentBinState);
    deactivateBinInhibitor();
}

static void actionBinEvalUnknown()
{
    indicateBinState(myCurrentBinState);
}

static void actionBinRefusedToBinAccepted()
{
    deactivateBinInhibitor();
}

static void actionBinUnknownToBinAccepted()
{
    deactivateBinInhibitor();
}

static void actionBinAcceptedToBinWeighing()
{
    TourManager_requestToAskWeight();
}

static void actionBinWeighingToBinEmptying()
{
    wat = Watchdog_construct(5 * 1000 * 1000, Speaker_timeOut);
    setTimeEmptyTime(wat);
    emptyBin();
}

static void actionBinEmptyingToWfs()
{
    TourManager_timeOutEmptyTime();
}

static void actionRestToStartActive()
{
    indicateEndOfRest();
}

static void actionRestToConnected()
{
    indicateEndOfRest();
}

static void actionRestToWfs()
{
    indicateEndOfRest();
}

static void actionRestToBinEval()
{
    indicateEndOfRest();
}

static void actionRestToBinRefused()
{
    indicateEndOfRest();
}

static void actionRestToBinAccepted()
{
    indicateEndOfRest();
}

static void actionRestToBinUnknown()
{
    indicateEndOfRest();
}

static void actionRestToBinWeighing()
{
    indicateEndOfRest();
}

static void actionRestToBinEmptying()
{
    indicateEndOfRest();
}

static void actionStartToRest()
{
    indicateRest();
}

static void actionConnectedToRest()
{
    indicateRest();
}

static void actionBinEvalToRest()
{
    indicateRest();
}

static void actionBinRefusedToRest()
{
    indicateRest();
}

static void actionBinUnknownToRest()
{
    indicateRest();
}

static void actionBinAcceptedToRest()
{
    indicateRest();
}

static void actionBinWeighingToRest()
{
    indicateRest();
}

static void actionBinEmptyingToRest()
{
    reset(wat);
    indicateRest();
}

//////////////////////////////////////////////
//////////     PUBLIC FUNCTIONS     //////////
//////////////////////////////////////////////

extern void Speaker_requestStartSignalOk()
{
    startBenno();
}

extern void Speaker_requestConnectionSignalOk()
{
    indicateConnectionOk();

    MqMsg msg = {.data.event = E_RE_CONNEXION_SIOK};
    mqSend(&msg);
}

extern void Speaker_requestDataReceptionSignalOk()
{
    for (int i = 0; i < 3; i++)
    {
        Leds_turnOnLed(leds[0]); //Turn on the led
        usleep(500 * 1000);
        Leds_turnOffLed(leds[0]); //Turn off the led
        usleep(500 * 1000);
    }
    
    MqMsg msg = {.data.event = E_RE_DATA_CONNEXIONOK};
    mqSend(&msg);
}

extern void Speaker_requestDataReceptionSignalKo()
{
    indicateStartOk();
}

extern void Speaker_requestBinStateSignal(BinState binState)
{
    myCurrentBinState = binState;
    MqMsg msg = {.data.event = E_RE_BIN_STATE};
    mqSend(&msg);
}

extern void Speaker_requestRestSignal()
{
    MqMsg msg = {.data.event = E_RE_REST};
    mqSend(&msg);
}

extern void Speaker_requestEndOfRestSignal()
{
    MqMsg msg = {.data.event = E_RE_ENDOFREST};
    mqSend(&msg);
}

extern void Speaker_requestToEmptyBin()
{
    MqMsg msg = {.data.event = E_RE_EMPTY_BIN};
    mqSend(&msg);
}

extern void Speaker_liftBin()
{
    if (inhibitor == false)
    {
        MqMsg msg = {.data.event = E_LIFT_BIN};
        mqSend(&msg);
    }
    else
    {
        // Do nothing
    }
}

extern void Speaker_forceLiftBin()
{
    MqMsg msg = {.data.event = E_FORCE_LIF_BIN};
    mqSend(&msg);
}

static void Speaker_timeOut(Watchdog *this)
{
    MqMsg msg = {.data.event = E_TO_EM_TIME};
    mqSend(&msg);
}

extern void Speaker_stopBenno()
{
    MqMsg msg = {.data.event = E_STOP_BENNO};
    StopButton_stopPolling();
    LiftingButton_stopPolling();
    ForcingButton_stopPolling();
    Leds_free();
    StopButton_free();
    LiftingButton_free();
    ForcingButton_free();
    TourManager_requestToStopTools();
    mqSend(&msg);
}

//////////////////////////////////////////////
//////////     STATIC FUNCTIONS     //////////
//////////////////////////////////////////////

static void evaluateBinState(BinState binState)
{
    MqMsg msg;
    switch (binState)
    {
    case BINSTATE_ACCEPTED:
        msg.data.event = E_EVAL_BINSTATE_ACCEPTED;
        break;
    case BINSTATE_REFUSED:
        msg.data.event = E_EVAL_BINSTATE_REFUSED;
        break;
    case BINSTATE_UNKNOWN:
        msg.data.event = E_EVAL_BINSTATE_UNKNOWN;
        break;
    default:;
    }
    mqSend(&msg);
}

static void timeOutEmptyTime()
{
    MqMsg msg = {.data.event = E_TO_EM_TIME};
    mqSend(&msg);
}
static void setTimeEmptyTime(Watchdog *timer)
{
    Watchdog_start(timer);
}
static void reset(Watchdog *timer)
{
    Watchdog_cancel(timer);
    Watchdog_destroy(timer);
}
static void init()
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
        PERROR("Error when try to remove the message queue named \n");
    }

    Leds_init();
    StopButton_init();
    LiftingButton_init();
    ForcingButton_init();
    leds = Leds_getLeds();
    myBal = mq_open(queueName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &attr);
    if (myBal == -1)
    {
        PERROR("Error when open the message queue (postbox)");
    }
}
static void indicateStartOk()
{
    for (int i = 0; i < 3; i++)
    {
        Leds_turnOnLed(leds[3]);  //Turn on the led
        usleep(500 * 1000);       //Green led remain enlighted during 3 secondes
        Leds_turnOffLed(leds[3]); //Turn off the led
        usleep(500 * 1000);
    }
}

static void indicateConnectionOk()
{
    sleep(3);
    Leds_turnOnLed(leds[3]);
}

static void indicateDataReceptionOk()
{
    
}

static void indicateDataReceptionKo()
{
    indicateStartOk();
}

static void indicateBinState(BinState binState)
{
    switch (binState)
    {
    case BINSTATE_ACCEPTED:;
        Leds_turnOnLed(leds[2]);
        sleep(3);                 //Green led remain enlighted during 3 secondes
        Leds_turnOffLed(leds[2]); //Turn off the led
        break;

    case BINSTATE_REFUSED:;
        Leds_turnOnLed(leds[1]);  //Turn on the red led
        sleep(3);                 //Red led remain enlighted during 3 secondes
        Leds_turnOffLed(leds[1]); //Turn off the red led
        break;
    case BINSTATE_UNKNOWN:;
        Leds_turnOnLed(leds[0]);  //Turn on the orange led
        sleep(3);                 //Orange led remain enlighted during 3 secondes
        Leds_turnOffLed(leds[0]); //Turn off the orange led
        break;
    default:
        break;
    }
}

static void indicateRest()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 6; j++)
    {
        Leds_turnOnLed(leds[3]);
        usleep(20 * 1000);
        Leds_turnOffLed(leds[3]);
        usleep(10 * 1000);
    }
    }
}

static void indicateEndOfRest()
{
    indicateConnectionOk();
}

static void emptyBin()
{
    deactivateBinInhibitor();
    activateBinInhibitor();
}

static void deactivateBinInhibitor()
{
    inhibitor = false;
}

static void activateBinInhibitor()
{
    inhibitor = true;
}

static void startBenno()
{
    int8_t check;
    init();
    check = pthread_create(&myThread, NULL, &run, NULL);
    if (check != 0)
    {
        PERROR("Error when create the thread");
    }
    StopButton_startPolling();
    LiftingButton_startPolling();
    ForcingButton_startPolling();
    indicateStartOk();
    activateBinInhibitor();
    pthread_detach(myThread);
}
