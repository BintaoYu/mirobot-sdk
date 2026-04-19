#pragma once

#include "comm_interface.h"
#include <deque>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace test_support {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline void fail(const char* file, int line, const std::string& message) {
    std::ostringstream oss;
    oss << file << ":" << line << " " << message;
    throw TestFailure(oss.str());
}

inline void check(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        fail(file, line, std::string("check failed: ") + expression);
    }
}

template <typename Actual, typename Expected>
void checkEqual(const Actual& actual, const Expected& expected, const char* actual_expr, const char* expected_expr, const char* file, int line) {
    if (!(actual == expected)) {
        std::ostringstream oss;
        oss << "check failed: " << actual_expr << " == " << expected_expr
            << " (actual: " << actual << ", expected: " << expected << ")";
        fail(file, line, oss.str());
    }
}

template <typename ExceptionType, typename Func>
void checkThrows(Func&& func, const char* expression, const char* expected_exception, const char* file, int line) {
    try {
        func();
    } catch (const ExceptionType&) {
        return;
    } catch (const std::exception& ex) {
        std::ostringstream oss;
        oss << "check failed: " << expression << " throws " << expected_exception
            << " (caught different std::exception: " << ex.what() << ")";
        fail(file, line, oss.str());
    } catch (...) {
        std::ostringstream oss;
        oss << "check failed: " << expression << " throws " << expected_exception
            << " (caught non-std exception)";
        fail(file, line, oss.str());
    }

    std::ostringstream oss;
    oss << "check failed: " << expression << " throws " << expected_exception
        << " (no exception thrown)";
    fail(file, line, oss.str());
}

struct TestCase {
    const char* name;
    std::function<void()> body;
};

inline int runTests(const std::vector<TestCase>& tests) {
    int failed = 0;

    for (const auto& test : tests) {
        try {
            test.body();
            std::cout << "[PASS] " << test.name << std::endl;
        } catch (const std::exception& ex) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": " << ex.what() << std::endl;
        } catch (...) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << ": unknown exception" << std::endl;
        }
    }

    if (failed == 0) {
        std::cout << "All " << tests.size() << " tests passed." << std::endl;
        return 0;
    }

    std::cerr << failed << " of " << tests.size() << " tests failed." << std::endl;
    return 1;
}

class MockComm final : public CommInterface {
public:
    void enqueueRead(std::string line) {
        active_reads_.push_back(std::move(line));
    }

    void enqueueResponseBatch(std::initializer_list<std::string> lines) {
        std::deque<std::string> batch;
        for (const auto& line : lines) {
            batch.push_back(line);
        }
        pending_batches_.push_back(std::move(batch));
    }

    const std::vector<std::string>& writes() const {
        return writes_;
    }

    void writeData(const std::string& data) override {
        writes_.push_back(data);
        if (active_reads_.empty() && !pending_batches_.empty()) {
            active_reads_ = std::move(pending_batches_.front());
            pending_batches_.pop_front();
        }
    }

    std::string readLine() override {
        if (active_reads_.empty()) {
            throw TestFailure("Unexpected readLine() with no queued data");
        }

        std::string line = active_reads_.front();
        active_reads_.pop_front();
        return line;
    }

    bool dataAvailable() override {
        return !active_reads_.empty();
    }

private:
    std::vector<std::string> writes_;
    std::deque<std::string> active_reads_;
    std::deque<std::deque<std::string>> pending_batches_;
};

} // namespace test_support

#define CHECK(expr) ::test_support::check((expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) ::test_support::checkEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define CHECK_THROWS_AS(expr, exception_type) \
    ::test_support::checkThrows<exception_type>([&]() { (void)(expr); }, #expr, #exception_type, __FILE__, __LINE__)
