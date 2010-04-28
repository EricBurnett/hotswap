#include <cstring>
#include <iostream>

using namespace std;

const char* VERSION = "0.1";

int main(int argc, char** argv) {
    cerr << "HotSwap example started - version " << VERSION << endl;

    if (argc == 3 && strcmp(argv[1], "--hotswapping") == 0) {
	cerr << "Hot Swapping\n";
    } else {
	// Initialize state
	cerr << "Initial load\n";
    }

    cerr << "Terminating\n";
    return 0;
}
