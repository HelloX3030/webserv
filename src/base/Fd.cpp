#include "base/Fd.hpp"

Fd::Fd()
    : fd(-1)
{
}

Fd::Fd(Fd &&other) noexcept
    : fd(other.fd)
{
    other.fd = -1;
}

Fd &Fd::operator=(Fd &&other) noexcept
{
    if (this != &other)
    {
        if (fd != -1)
            close(fd);

        fd = other.fd;
        other.fd = -1;
    }
    return *this;
}

Fd::~Fd()
{
    if (fd != -1)
    {
        close(fd);
    }
}

Fd::Fd(int fd)
    : fd(fd)
{
}

void Fd::set(int fd)
{
    if (this->fd != -1)
    {
        close(this->fd);
    }
    this->fd = fd;
}

int Fd::get() const
{
    return fd;
}

std::string Fd::to_string() const
{
    return std::to_string(get());
}

std::ostream &operator<<(std::ostream &os, const Fd &fd)
{
    return os << fd.get();
}
