#include <signal.h>
#include <sys/poll.h>

#include <chrono>
#include <string>

#include "ida100.h"

// interval for logging reading to stderr
uint64_t LOG_INTERVAL_MS = 1000;

IDA100 loadCell;

// in microseconds
uint64_t lastLogTime = 0;

void closeLoadCell(int signal) {
    std::cerr << "Closing load cell (signal " << signal << ")" << std::endl;
    loadCell.close();
    exit(1);
}

void registerSignalHandlers() {
    struct sigaction sg;
    sg.sa_handler = closeLoadCell;
    sigfillset(&sg.sa_mask);  // block all signals while in the handler
    sg.sa_flags = 0;

    sigaction(SIGINT, &sg, nullptr);
    sigaction(SIGTERM, &sg, nullptr);
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

    registerSignalHandlers();

    loadCell.calibrate();

    while (true) {
        checkForCalibration();

        uint64_t timestamp = micros();
        double lbs = loadCell.read() / ticksPerPound;

        std::cout << "rec: " << timestamp << " " << lbs << std::endl;

        if (timestamp - lastLogTime > LOG_INTERVAL_MS * 1000) {
            std::cerr << "rec: " << timestamp << " " << lbs << std::endl;
            lastLogTime = timestamp;
        }
    }
}
