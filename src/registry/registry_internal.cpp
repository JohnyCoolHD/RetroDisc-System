#include "registry_internal.hpp"

#include "registry.hpp"
#include "context.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

namespace
{


/*
    ================================================================
    HOME
    ================================================================
*/

std::filesystem::path getHome()
{
    const char* home =
        std::getenv("HOME");

    if(
        home == nullptr ||
        *home == '\0'
    )
    {
        return {};
    }

    return std::filesystem::path(home);
}


} // namespace


/*
    ================================================================
