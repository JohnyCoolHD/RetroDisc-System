#include "runtime_internal.hpp"
#include "context.hpp"

#include <string>


namespace runtime_internal
{


/*
    ================================================================
    RUNTIME USER
    ================================================================
*/

std::string determineRuntimeUser(
    const Context&
)
{
    /*
        Linux account and Windows/Wine account are deliberately
        different concepts.

        Linux:
            maxim

        Windows/Wine/Proton:
            RetroDisc

        RetroDisc is the ONLY canonical Windows profile.
    */

    return CANONICAL_WINDOWS_USER;
}


} // namespace runtime_internal
