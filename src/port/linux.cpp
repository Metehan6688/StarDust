#include "config.hpp"


#ifdef USE_LINUX_FRAMEWORK

#include "linux.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>


namespace {
    speed_t toTermiosBaud(uint32_t baud) {
        switch (baud) {
            case 9600:   return B9600;
            case 19200:  return B19200;
            case 38400:  return B38400;
            case 57600:  return B57600;
            case 115200: return B115200;
            case 230400: return B230400;
            default:     return B115200;
        }
    }
}


namespace starDustNS::port {
    linuxSerialPort::linuxSerialPort(const char* devicePath, uint32_t baudRate) {
        fd_ = ::open(devicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) {
            return;
        }

        termios tty{};
        if (tcgetattr(fd_, &tty) != 0) {
            ::close(fd_);
            fd_ = -1;
            return;
        }

        speed_t baud = toTermiosBaud(baudRate);
        cfsetispeed(&tty, baud);
        cfsetospeed(&tty, baud);

        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag &= ~CRTSCTS;
        tty.c_cflag |= CREAD | CLOCAL;

        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;
        tty.c_lflag &= ~ECHOE;
        tty.c_lflag &= ~ISIG;

        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

        tty.c_oflag &= ~OPOST;
        tty.c_oflag &= ~ONLCR;

        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = 0;

        tcsetattr(fd_, TCSANOW, &tty);
    }

    linuxSerialPort::~linuxSerialPort() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    size_t linuxSerialPort::writeByte(const uint8_t* data, size_t len) {
        if (fd_ < 0) return 0;
        ssize_t written = ::write(fd_, data, len);
        return written < 0 ? 0 : static_cast<size_t>(written);
    }

    bool linuxSerialPort::readByte(uint8_t& outByte) {
        if (fd_ < 0) return false;
        ssize_t n = ::read(fd_, &outByte, 1);
        return n == 1;
    }

}

#endif
