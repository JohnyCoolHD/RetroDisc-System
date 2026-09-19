#include "runtime_internal.hpp"
#include "runtime.hpp"
#include "context.hpp"

#include <filesystem>


using namespace runtime_internal;


std::filesystem::path resolveProton(const Context& ctx)
{
    return findProton(ctx);
}
