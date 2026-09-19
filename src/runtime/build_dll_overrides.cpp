#include "runtime_internal.hpp"
#include "context.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    DLL OVERRIDES
    ================================================================
*/

std::string buildDllOverrides(
    const Context& ctx
)
{
    if(ctx.wine.dllOverrides.empty())
    {
        return {};
    }

    std::ostringstream value;

    bool first = true;

    for(const auto& [dll, overrideValue] :
        ctx.wine.dllOverrides)
    {
        if(dll.empty())
        {
            continue;
        }

        if(!first)
        {
            value << ";";
        }

        value
            << dll
            << "="
            << overrideValue;

        first = false;
    }

    return value.str();
}


} // namespace runtime_internal
