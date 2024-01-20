#include <signal.h>
#include <sys/poll.h>

#include <chrono>

#include "ida100.h"

const char* SERIAL_NUMBER_1 = "652964";
const char* SERIAL_NUMBER_2 = "1076702";

const double TICKS_PER_POUND_1 = 10.0 / 2000.0 * 100000.0 * 3.5;
const double TICKS_PER_POUND_2 = 10.0 / 2000.0 * 100000.0 * 3.5;

IDA100 loadCell1(SERIAL_NUMBER_1, TICKS_PER_POUND_1);
IDA100 loadCell2(SERIAL_NUMBER_2, TICKS_PER_POUND_2);

void sigintHandler(int sig) {
    std::cout << "Caught SIGINT, closing devices" << std::endl;
    loadCell1.close();
    loadCell2.close();
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
            loadCell1.calibrate();
            loadCell2.calibrate();
        }
    }
}

int main() {
    signal(SIGINT, sigintHandler);

    loadCell1.calibrate();
    loadCell2.calibrate();

    while (true) {
        checkForCalibration();

        uint64_t timestamp = micros();

        double lbs1 = loadCell1.readLbs();
        double lbs2 = loadCell2.readLbs();

        std::cout << "rec: " << timestamp << " " << lbs1 << " " << lbs2
                  << std::endl;
    }
}
