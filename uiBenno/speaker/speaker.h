/**
 * @file speaker.h
 * @author Adrien Leroux
 * @brief fichier header de speaker.c
 * @version 0.1
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company . All rights reserved.
 * 
 */

#ifndef SPEAKER_H
#define SPEAKER_H
typedef enum
{
    BINSTATE_ACCEPTED,
    BINSTATE_REFUSED,
    BINSTATE_UNKNOWN,

} BinState;

static const char *const binStateName[] = {
    "BINSTATE_ACCEPTED",
    "BINSTATE_REFUSED",
    "BINSTATE_UNKNOWN"
};

static const char *binStateGetName(int8_t i)
{
    return binStateName[i];
}
/**
 * @brief TourManager ask Speaker to indicate that the Benno's starting has been successful
 * 
 */
extern void Speaker_requestStartSignalOk();

/**
 * @brief TourManager ask Speaker to indicate that the connexion between Benno and visio has been successful
 * 
 */
extern void Speaker_requestConnectionSignalOk();

/**
 * @brief TourManager ask Speaker to indicate that Benno has recepted data.
 * 
 */ 
extern void Speaker_requestDataReceptionSignalOk();

/**
 * @brief TourManager ask Speaker to indicate that there is a connexion error between Benno and Visio.
 * 
 */
extern void Speaker_requestDataReceptionSignalKo();

/**
 * @brief TourManager ask Speaker to indicate the bin state when it is positionned to be lifted.
 * 
 * @param binState 
 */
extern void Speaker_requestBinStateSignal(BinState binState);

/**
 * @brief TourManager ask Speaker to indicate the system rest state.
 * 
 */
extern void Speaker_requestRestSignal();

/**
 * @brief TourManager ask Speaker to indicate the end of the rest.
 * 
 */
extern void Speaker_requestEndOfRestSignal();

/**
 * @brief TourManager ask Speaker to simulate the emptying of the bin
 * 
 */
extern void Speaker_requestToEmptyBin();

/**
 * @brief Allow to simulate the emptying of the bin
 * 
 */
extern void Speaker_liftBin();

/**
 * @brief force the lifting of the bin and deactivate the inhibitor
 * 
 */
extern void Speaker_forceLiftBin();

/**
 * @brief 
 * 
 */
extern void Speaker_stopBenno();

#endif /* SPEAKER_H */