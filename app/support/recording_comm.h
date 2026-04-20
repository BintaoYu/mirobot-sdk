#pragma once

#include "comm_interface.h"
#include <deque>
#include <string>
#include <utility>
#include <vector>

class RecordingComm final : public CommInterface {
public:
    void queueResponse(std::string line) {
        responses_.push_back(std::move(line));
    }

    const std::vector<std::string>& writes() const {
        return writes_;
    }

    void writeData(const std::string& data) override {
        writes_.push_back(data);
    }

    std::string readLine() override {
        if (responses_.empty()) {
            return {};
        }

        std::string line = std::move(responses_.front());
        responses_.pop_front();
        return line;
    }

    bool dataAvailable() override {
        return !responses_.empty();
    }

private:
    std::vector<std::string> writes_;
    std::deque<std::string> responses_;
};
