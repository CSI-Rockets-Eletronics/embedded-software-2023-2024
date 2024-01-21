#include <signal.h>
#include <sys/poll.h>

#include <chrono>
#include <string>

#include "ida100.h"

IDA100 loadCell;

void sigintHandler(int sig) {
    std::cout << "Caught SIGINT, closing devices" << std::endl;
    loadCell.close();
    exit(sig);
}

uint64_t micros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

void checkForCalibration() {
    struct pollfd fds;
    fds.fd = 0;  // stdin
    fds.events = POLLIN;

    if (poll(&fds, 1, 0)) {
        std::string message;
        std::cin >> message;

        if (message == "calibrate") {
            loadCell.calibrate();
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0]
                  << " <serial number> <ticks per pound>" << std::endl;
        exit(1);
    }

    char* serialNumber = argv[1];
    double ticksPerPound = std::stod(argv[2]);

    loadCell.open(serialNumber);

    signal(SIGINT, sigintHandler);

    loadCell.calibrate();

    while (true) {
        checkForCalibration();

        uint64_t timestamp = micros();
        double lbs = loadCell.read() / ticksPerPound;

        std::cout << "rec: " << timestamp << " " << lbs << std::endl;
    }
}
