#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "hotswap.h"

using namespace std;

const char* VERSION	    = "0.1";
const char* HOTSWAP_COMMAND = "--hotswapping";

void init(int argc, char** argv, ProgramState& state) {
    state.set_cur_state(ProgramState::STATE_INIT);
}

ReturnCode runStateMachine(ProgramState& state) {
    cerr << "Running state machine\n";

    // Put stdin into non-blocking, raw mode, so we can watch for character
    // input one keypress at a time.
    setStdinBlocking(false);

    while (true) {
	ProgramState::State next;
	switch (state.cur_state()) {
	    case ProgramState::STATE_INIT:
		next = runState_init(state);
		break;

	    case ProgramState::STATE_PROCESS_LINE:
		next = runState_process_line(state);
		break;

	    case ProgramState::STATE_DONE:
		setStdinBlocking(true);
		return SUCCESS;

	    case ProgramState::STATE_NONE:
	    case ProgramState::STATE_ERROR:
	    default:
		setStdinBlocking(true);
		return FAILURE;
	}

	ProgramState::State cur = state.cur_state();
	state.set_prev_state(cur);
	state.set_cur_state(next);

	// For now, simply let the user decide when to swap and quit. We can
	// always change this later.
	ReturnCode code = checkForUserSignal();
	if (code != CONTINUE) {
	    setStdinBlocking(true);
	    return code;
	}
    }
}

ProgramState::State runState_init(ProgramState& state) {
    cout << "Please provide a line of text for me to repeat ad-nauseum\n";
    string line;
    setStdinBlocking(true);
    getline(cin, line);
    setStdinBlocking(false);
    state.set_line_text(line);
    cout << "Thanks!\n";
    state.set_line_count(0);

    return ProgramState::STATE_PROCESS_LINE;
}

ProgramState::State runState_process_line(ProgramState& state) {
    int count = state.line_count();
    string line = state.line_text();
    cout << count << ": " << line << endl;
    state.set_line_count(count+1);
    sleep(1);

    return ProgramState::STATE_PROCESS_LINE;
}

ReturnCode checkForUserSignal() {
    // We had better be in non-blocking mode, or this is going to bring
    // everything to a screeching halt.
    char c;
    while ((c = getc(stdin)) != EOF) {
	if (c == 'u') {
	    cout << endl;
	    return SWAP;
	} else if (c == 'q') {
	    cout << endl;
	    return SUCCESS;
	}
    }
    return CONTINUE;
}

void setStdinBlocking(bool block) {
    const int fd = fileno(stdin);
    termios flags;
    tcgetattr(fd,&flags);
    flags.c_lflag &= ~ICANON; // set raw (unset canonical modes)
    flags.c_cc[VMIN] = block? 1 : 0; // i.e. min 1 char for blocking, 0 chars for non-blocking
    flags.c_cc[VTIME] = 0; // block if waiting for char
    tcsetattr(fd,TCSANOW,&flags);
}

//------------------------------------------------------------------------------
// The code from here down is framework code, to handle the work of swapping
// binaries. No need to touch it generation to generation.

int main(int argc, char** argv) {
    cerr << "HotSwap example started - version " << VERSION << endl;
    
    ProgramState state;
    if (argc == 4 && strcmp(argv[1], HOTSWAP_COMMAND) == 0) {
	cerr << "Coming up from swap!\n";

	// Read the state in from the associated pipe.
	int fd = atoi(argv[2]);
	int child = atoi(argv[3]);
	if (!state.ParseFromFileDescriptor(fd)) {
	    // Unable to read from pipe?! Oh well, we will default to the 
	    // ERROR state in that case.
	    cerr << "ERROR: unable to read from previous generation.\n";
	}
	close(fd);

	// Wait on child termination, so we don't leave zombie processes 
	// hanging around.
	int status = 0;	
	waitpid(child, &status, 0);
    } else {
	// Initialize state
	cerr << "Initial call\n";
	state.set_initial_version(VERSION);
	init(argc, argv, state);	
    }

    int code = -1;
    switch(runStateMachine(state)) {
	case SUCCESS:
	    code = 0;
	    break;
	case FAILURE:
	    code = -1;
	    break;
	case SWAP:
	    // This does not return.
	    swapToNewVersion(argv[0], state);
	    break;
    }

    cerr << "Terminating with code " << code << endl;
    return code;
}

void swapToNewVersion(char* path, ProgramState& state) {
    cerr << "Going down for swap\n";
    int fds[2];
    pipe(fds);
    int reader = fds[0];
    int writer = fds[1];

    int childPid;
    if (childPid = fork()) {
	// Parent - becomes the new version.
	close(writer);

	char fdString[20] = {0};
	sprintf(fdString, "%d", reader);
	char pidString[20] = {0};
	sprintf(pidString, "%d", childPid);
	execl(path, path, HOTSWAP_COMMAND, fdString, pidString, NULL);
	
	cerr << "Exec failed!\n" <<
	    "Really, we should just keep going with this version here, but\n" <<
	    "I'm not going to bother, sorry. Goodbye....\n";
	exit(1);
    } else {
	// Child - passes state back and then quits.
	close(reader);
	state.SerializeToFileDescriptor(writer);
	close(writer);
	exit(0);
    }
}

