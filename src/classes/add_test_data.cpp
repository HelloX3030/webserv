#include "WebServ.hpp"

namespace WebServ
{

void add_test_data()
{
    std::cout << format::header("ADD TEST DATA") << std::endl;

    Listener::listener.emplace_back(0, 8000);
    Listener::listener.emplace_back(0, 8100);
    Listener::listener.emplace_back(0, 8200);
    Listener::listener.emplace_back(0, 8300);
}

}; // namespace WebServ
