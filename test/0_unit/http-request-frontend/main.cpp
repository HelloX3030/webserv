#include "test_harness.hpp"

#include <cstring>

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-v") == 0)
            verbose = true;
    }
    return run_all_tests(verbose);
}
