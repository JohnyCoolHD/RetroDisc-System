#include "prefix_internal.hpp"
#include "registry.hpp"
#include "context.hpp"


using namespace prefix_internal;


/*
    ================================================================
    PUBLIC PREFIX ENTRY POINT
    ================================================================
*/


bool preparePrefix(
    Context& ctx
)
{
    return prepareGamePrefix(
        ctx
    );
}
