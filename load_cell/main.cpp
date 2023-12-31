#include <signal.h>
#include <sys/poll.h>

#include <chrono>

#include "ida100.h"

const PVOID DEV_INDEX = 0;

const double TICKS_PER_POUND = 10.0 / 2000.0 * 100000.0 * 3.5;

IDA100 ida100(DEV_INDEX, TICKS_PER_POUND);

void sigintHandler(int sig) {
    std::cout << "Caught SIGINT, closing device" << std::endl;
    ida100.close();
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
            ida100.calibrate();
        }
    }
}

int main() {
    signal(SIGINT, sigintHandler);

    ida100.calibrate();

    while (true) {
        checkForCalibration();

        double lbs = ida100.readLbs();

        uint64_t timestamp = micros();

        std::cout << "rec: " << timestamp << " " << lbs << std::endl;
    }
}
