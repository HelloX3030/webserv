#pragma once

#include "base/base.hpp"

namespace Listener
{

extern int size;
extern std::vector<int> server_id;
extern std::vector<int> port;
extern std::vector<int> fd;

void add(int new_server_id, int new_port);
int init();
void quit();

} // namespace Listener
