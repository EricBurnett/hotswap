#ifndef HOTSWAP_H
#define HOTSWAP_H

#include "hotswap.pb.h"

enum ReturnCode {
    CONTINUE,
    SUCCESS,
    FAILURE,
    SWAP
};

// Initializes the program state as necessary from the command line arguments.
void init(int argc, char** argv, ProgramState& state);

// Runs the state machine specified by ProgramState, looping through the states
// until one of the special states is encountered (NONE, ERROR, DONE) or the
// signal to swap to a new binary is set.
ReturnCode runStateMachine(ProgramState& state);

// Fetches some initial data from the user to demonstrate state persistence.
ProgramState::State runState_init(ProgramState& state);

// Simply writes demonstration data to the console, sleeps, and loops back on
// its own state.
ProgramState::State runState_process_line(ProgramState& state);

// For now we simply watch console input, updating on "u" and quitting on "q"
// and ignoring all other characters.
ReturnCode checkForUserSignal();

// Turns stdin blocking on or off.
void setStdinBlocking(bool block);


//------------------------------------------------------------------------------
// The methods below are framework methods, and can probably be ignored.

// Main program entrypoint. 
int main(int argc, char** argv);

// Swaps to a new binary version. This method does not return.
void swapToNewVersion(char* path, ProgramState& state);

#endif
