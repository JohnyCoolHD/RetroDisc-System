#include "filesystem_internal.hpp"

#include <cstdlib>
#include <iostream>

#include <sys/wait.h>


/*
    ================================================================
    SHELL QUOTING
    ================================================================
*/

std::string shellQuote(
    const std::string& value
)
{
    std::string result = "'";

    for(const char c : value)
    {
        if(c == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += c;
        }
    }

    result += "'";

    return result;
}


/*
    ================================================================
    RUN COMMAND
    ================================================================
*/

bool runCommand(
    const std::string& command,
    bool showCommand
)
{
    if(showCommand)
    {
        std::cout
            << command
            << std::endl;
    }

    const int result =
        std::system(
            command.c_str()
        );

    if(result == -1)
    {
        return false;
    }

    if(!WIFEXITED(result))
    {
        return false;
    }

    return WEXITSTATUS(result) == 0;
}