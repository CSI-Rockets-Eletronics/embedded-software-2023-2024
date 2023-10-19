// share (via copying) across both boards

#ifndef SENTENCE_SERIAL_H_
#define SENTENCE_SERIAL_H_

#include <Arduino.h>

class SentenceSerial {
   private:
    static const int BAUD = 115200;

    static const int MAX_SENTENCE_LEN = 64;
    static const char SENTENCE_START = '<';
    static const char SENTENCE_END = '>';

    HardwareSerial &serial;
    std::function<void(const char *)> processCompletedSentence;

    String curSentence;
    bool sawStart = false;

   public:
    SentenceSerial(HardwareSerial &serial,
                   std::function<void(const char *)> processCompletedSentence)
        : serial(serial), processCompletedSentence(processCompletedSentence) {}

    void sendSentence(const char *sentence) {
        serial.print(SENTENCE_START);
        serial.print(sentence);
        serial.print(SENTENCE_END);
    }

    void init(int rxPin, int txPin) {
        serial.begin(BAUD, SERIAL_8N1, rxPin, txPin);
        curSentence.reserve(MAX_SENTENCE_LEN + 1);  // +1 for null terminator
    }

    void tick() {
        while (serial.available()) {
            char c = serial.read();

            if (c == SENTENCE_START) {
                curSentence = "";
                sawStart = true;
            } else if (c == SENTENCE_END) {
                processCompletedSentence(curSentence.c_str());
                sawStart = false;
            } else if (sawStart) {
                bool success = curSentence.concat(c);
                if (!success) {
                    Serial.println(
                        "Reading character from scientific serial failed"
                        " (sentence too long?)");
                }
            }  // else ignore c because we haven't seen a start yet
        }
    }
};

#endif  // SENTENCE_SERIAL_H_