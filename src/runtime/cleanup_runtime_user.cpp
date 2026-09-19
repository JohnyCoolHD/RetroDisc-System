#include "runtime_internal.hpp"
#include "context.hpp"

#include <string>


namespace runtime_internal
{


/*
    ================================================================
    CLEANUP RUNTIME USER
    ================================================================
*/

void cleanupRuntimeUser(
    const Context&,
    const std::string&
)
{
    /*
        There is no temporary runtime user.

        The persistent RetroDisc profile must NEVER be deleted.
    */
}


} // namespace runtime_internal
