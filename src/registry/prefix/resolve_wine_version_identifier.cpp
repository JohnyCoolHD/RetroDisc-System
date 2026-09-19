#include "prefix_internal.hpp"

#include <array>
#include <string>


namespace prefix_internal
{


/*
    ================================================================
    RESOLVE WINE VERSION IDENTIFIER
    ================================================================
*/


std::string resolveWineVersionIdentifier()
{
    std::array<char, 256> buffer{};
    std::string output;


    FILE* pipe =
        popen(
            "wine --version 2>/dev/null",
            "r"
        );


    if(pipe == nullptr)
    {
        return "default";
    }


    while(
        fgets(
            buffer.data(),
            static_cast<int>(buffer.size()),
            pipe
        ) != nullptr
    )
    {
        output +=
            buffer.data();
    }


    const int status =
        pclose(pipe);


    if(
        status != 0 ||
        output.empty()
    )
    {
        return "default";
    }


    return sanitizeIdentifier(
        output
    );
}


} // namespace prefix_internal
