#include "runtime_internal.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    WINE REGISTRY
    ================================================================
*/

void appendWineRegAdd(
    std::ostringstream& command,
    const std::string& key,
    const std::string& valueName,
    const std::string& type,
    const std::string& value
)
{
    command
        << "wine reg add "
        << shellQuote(key)
        << " /v "
        << shellQuote(valueName)
        << " /t "
        << shellQuote(type)
        << " /d "
        << shellQuote(value)
        << " /f && ";
}


} // namespace runtime_internal
