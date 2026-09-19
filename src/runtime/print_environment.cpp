#include "runtime_internal.hpp"
#include "context.hpp"

#include <iostream>


namespace runtime_internal
{


/*
    ================================================================
    PRINT ENVIRONMENT
    ================================================================
*/

void printEnvironment(
    const Context& ctx
)
{
    for(const auto& [key, value] : ctx.environment)
    {
        std::cout
            << "Environment: "
            << key
            << "="
            << value
            << std::endl;
    }
}


} // namespace runtime_internal
