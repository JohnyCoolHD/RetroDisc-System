#include "runtime_internal.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


void appendWineRegDelete(
    std::ostringstream& command,
    const std::string& key,
    const std::string& valueName
)
{
    command
        << "wine reg delete "
        << shellQuote(key)
        << " /v "
        << shellQuote(valueName)
        << " /f 2>/dev/null || true && ";
}


} // namespace runtime_internal
