#pragma once

#include <iostream>
#include <string>
#include <unistd.h>

class Fd
{
  private:
    int fd;

  public:
    Fd();
    Fd(const Fd &other) = delete;
    Fd &operator=(const Fd &other) = delete;
    Fd(Fd &&other) noexcept;
    Fd &operator=(Fd &&other) noexcept;
    ~Fd();

    // Special Constructor
    explicit Fd(int fd);

    // Functions
    void set(int fd);
    int get() const;
    std::string to_string() const;
};

std::ostream &operator<<(std::ostream &os, const Fd &fd);
