#include "WebServ.hpp"

namespace WebServ
{

void add_test_data()
{
    logging::log(FUNCTION, "WebServ::add_test_data()");

    add_listener(8000);
    add_listener(8100);
    add_listener(8200);
    add_listener(8300);
}

}; // namespace WebServ
