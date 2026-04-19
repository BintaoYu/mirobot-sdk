#include "serial_comm.h"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

constexpr std::intptr_t kInvalidSerialHandle = -1;

#ifdef _WIN32
HANDLE nativeHandle(std::intptr_t handle) {
    return reinterpret_cast<HANDLE>(handle);
}

std::string windowsErrorMessage(DWORD error_code) {
    LPSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD size = FormatMessageA(
        flags,
        nullptr,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);

    if (size == 0 || buffer == nullptr) {
        return "code " + std::to_string(error_code);
    }

    std::string message(buffer, size);
    LocalFree(buffer);

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    return message;
}
#else
int nativeHandle(std::intptr_t handle) {
    return static_cast<int>(handle);
}
#endif

} // namespace

SerialComm::SerialComm(std::string port) : port(std::move(port)), handle(kInvalidSerialHandle) {
#ifdef _WIN32
    HANDLE serial_handle = CreateFileA(
        this->port.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (serial_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port: " << windowsErrorMessage(GetLastError()) << std::endl;
        return;
    }

    handle = reinterpret_cast<std::intptr_t>(serial_handle);
#else
    handle = open(this->port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (handle < 0) {
        std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
        return;
    }
#endif

    configurePort();
}

bool SerialComm::isOpen() const {
    return handle != kInvalidSerialHandle;
}

SerialComm::~SerialComm() {
    if (!isOpen()) {
        return;
    }

#ifdef _WIN32
    CloseHandle(nativeHandle(handle));
#else
    close(nativeHandle(handle));
#endif
}

void SerialComm::configurePort() {
    if (!isOpen()) {
        return;
    }

#ifdef _WIN32
    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(nativeHandle(handle), &dcb)) {
        std::cerr << "Error getting serial port configuration: " << windowsErrorMessage(GetLastError()) << std::endl;
        return;
    }

    dcb.BaudRate = CBR_38400;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    if (!SetCommState(nativeHandle(handle), &dcb)) {
        std::cerr << "Error setting serial port configuration: " << windowsErrorMessage(GetLastError()) << std::endl;
        return;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(nativeHandle(handle), &timeouts)) {
        std::cerr << "Error setting serial timeouts: " << windowsErrorMessage(GetLastError()) << std::endl;
    }
#else
    struct termios tty;
    if (tcgetattr(nativeHandle(handle), &tty) != 0) {
        std::cerr << "Error getting terminal attributes: " << strerror(errno) << std::endl;
        return;
    }

    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN] = 1;                         // read doesn't block
    tty.c_cc[VTIME] = 1;                        // 0.1 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);    // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(nativeHandle(handle), TCSANOW, &tty) != 0) {
        std::cerr << "Error setting terminal attributes: " << strerror(errno) << std::endl;
    }
#endif
}

void SerialComm::writeData(const std::string &data) {
    if (!isOpen()) {
        return;
    }

#ifdef _WIN32
    DWORD bytes_written = 0;
    if (!WriteFile(nativeHandle(handle), data.data(), static_cast<DWORD>(data.size()), &bytes_written, nullptr)) {
        std::cerr << "Error writing to serial port: " << windowsErrorMessage(GetLastError()) << std::endl;
    }
#else
    const ssize_t bytes_written = write(nativeHandle(handle), data.c_str(), data.size());
    if (bytes_written < 0) {
        std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
    }
#endif
}

std::string SerialComm::readLine() {
    if (!isOpen()) {
        return "";
    }

    std::string result;
    char buf;
    while (true) {
#ifdef _WIN32
        DWORD bytes_read = 0;
        if (!ReadFile(nativeHandle(handle), &buf, 1, &bytes_read, nullptr)) {
            std::cerr << "Error reading from serial port: " << windowsErrorMessage(GetLastError()) << std::endl;
            break;
        }
        if (bytes_read == 0) {
            break;
        }
#else
        const ssize_t bytes_read = read(nativeHandle(handle), &buf, 1);
        if (bytes_read < 0) {
            std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
            break;
        }
        if (bytes_read == 0) {
            break;
        }
#endif
        if (buf == '\n') {
            break;
        }
        if (buf != '\r') {
            result += buf;
        }
    }
    return result;
}

bool SerialComm::dataAvailable() {
    if (!isOpen()) {
        return false;
    }

#ifdef _WIN32
    COMSTAT status = {};
    DWORD errors = 0;
    if (!ClearCommError(nativeHandle(handle), &errors, &status)) {
        std::cerr << "Error checking data availability: " << windowsErrorMessage(GetLastError()) << std::endl;
        return false;
    }
    return status.cbInQue > 0;
#else
    int bytes = 0;
    if (ioctl(nativeHandle(handle), FIONREAD, &bytes) == -1) {
        std::cerr << "Error checking data availability: " << strerror(errno) << std::endl;
        return false;
    }
    return bytes > 0;
#endif
}

void SerialComm::flush() {
    if (!isOpen()) {
        return;
    }

#ifdef _WIN32
    if (!PurgeComm(nativeHandle(handle), PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        std::cerr << "Error flushing serial port: " << windowsErrorMessage(GetLastError()) << std::endl;
    }
#else
    tcflush(nativeHandle(handle), TCIOFLUSH);
#endif
}
