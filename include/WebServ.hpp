#pragma once

#include "base/base.hpp"
#include "classes/Config.hpp"
#include "classes/Connection.hpp"
#include "classes/Server.hpp"

namespace WebServ
{
extern std::vector<ServerConfig> servers;

// Functions
[[nodiscard]] std::vector<ServerConfig> parse(int argc, char **argv);
void init(const std::vector<ServerConfig> &configs);
void display();
void run();
void quit();

} // namespace WebServ

/* on fn name change: `parse` -> `load_config`:

it's not really a parsing fn —
it is an initialisation step that happens to invoke a parser.
Its telos is: "bring the server configuration into existence from program arguments."
The name parse was inherited from the pipeline (init, parse, run, quit)
but it undersold what the fn actually does.

`load_config` states that configuration is being loaded
from an external source into the program's state (WebServ::servers)
*/
