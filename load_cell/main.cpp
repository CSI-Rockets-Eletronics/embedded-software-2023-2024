#include <signal.h>
#include <sys/poll.h>

#include <chrono>
#include <string>

#include "ida100.h"

IDA100 loadCell;

void closeLoadCell(int signal) {
    std::cerr << "Closing load cell" << std::endl;
    loadCell.close();
    exit(0);
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
        std::cerr << "Usage: " << argv[0]
                  << " <serial number> <ticks per pound>" << std::endl;
        exit(1);
    }

    char* serialNumber = argv[1];
    double ticksPerPound = std::stod(argv[2]);

    loadCell.open(serialNumber);

    signal(SIGINT, closeLoadCell);
    signal(SIGTERM, closeLoadCell);

    loadCell.calibrate();

    while (true) {
        checkForCalibration();

        uint64_t timestamp = micros();
        double lbs = loadCell.read() / ticksPerPound;

        std::cout << "rec: " << timestamp << " " << lbs << std::endl;
    }
}
