#pragma once

#include "config.hpp"

#ifdef USE_LINUX_FRAMEWORK

#include "port.hpp"
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

namespace starDustNS::port{

    namespace detail {
        inline speed_t toTermiosBaud(uint32_t baud) {
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

    class linuxSerialPort : public internalPort {
        public:
            explicit linuxSerialPort(const char* devicePath, uint32_t baudRate = 115200) {
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

                speed_t baud = detail::toTermiosBaud(baudRate);
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

            ~linuxSerialPort() override {
                if (fd_ >= 0) {
                    ::close(fd_);
                }
            }

            linuxSerialPort(const linuxSerialPort&) = delete;
            linuxSerialPort& operator=(const linuxSerialPort&) = delete;

            size_t writeByte(const uint8_t* data, size_t len) override {
                if (fd_ < 0) return 0;
                ssize_t written = ::write(fd_, data, len);
                return written < 0 ? 0 : static_cast<size_t>(written);
            }

            bool readByte(uint8_t& outByte) override {
                if (fd_ < 0) return false;
                ssize_t n = ::read(fd_, &outByte, 1);
                return n == 1;
            }

            bool isOpen() const { return fd_ >= 0; }

        private:
            int fd_ = -1;
    };
}

#endif
