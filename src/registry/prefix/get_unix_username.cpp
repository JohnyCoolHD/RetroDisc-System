#include "prefix_internal.hpp"

#include <string>
#include <pwd.h>
#include <unistd.h>


namespace prefix_internal
{


std::string getUnixUsername()
{
    const auto uid =
        ::getuid();

    const auto* password =
        ::getpwuid(uid);

    if(
        password == nullptr ||
        password->pw_name == nullptr ||
        *password->pw_name == '\0'
    )
    {
        return {};
    }


    return std::string(
        password->pw_name
    );
}


} // namespace prefix_internal
