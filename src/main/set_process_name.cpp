#include "main_internal.hpp"

#include <string>
#include <sys/prctl.h>


namespace main_internal
{


/*
    ================================================================
    LINUX PROCESS NAME
    ================================================================
*/

void setProcessName(
    const std::string& gameName
)
{
    std::string processName =
        gameName.substr(
            0,
            15
        );

    if(processName.empty())
    {
        processName =
            "RetroDisc";
    }

    prctl(
        PR_SET_NAME,
        processName.c_str(),
        0,
        0,
        0
    );
}


} // namespace main_internal
