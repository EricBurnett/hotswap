#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/wait.h>

#include "hotswap.h"
#include "hotswap.pb.h"

using namespace std;

const char* VERSION	    = "0.1";
const char* HOTSWAP_COMMAND = "--hotswapping";

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
	state.set_cur_state(ProgramState::STATE_INIT);
	state.set_initial_version(VERSION);	
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

ReturnCode runStateMachine(ProgramState& state) {
    cerr << "Running state machine\n";

    char c = getchar();
    if (c == 'u') {
	return SWAP;
    } else if (c == 'e') {
	return FAILURE;
    }

    return SUCCESS;
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

