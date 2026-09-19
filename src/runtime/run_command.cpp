#include "runtime_internal.hpp"

#include <cstdlib>
#include <iostream>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    COMMAND EXECUTION
    ================================================================
*/

int runCommand(
    const std::string& command
)
{
    std::cout
        << std::endl
        << "Executing:"
        << std::endl
        << "    "
        << command
        << std::endl
        << std::endl;

    return std::system(
        command.c_str()
    );
}


} // namespace runtime_internal
