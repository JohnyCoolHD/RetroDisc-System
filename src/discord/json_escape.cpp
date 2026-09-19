#include "discord_internal.hpp"

#include <string>


namespace discord_internal
{


/*
    ================================================================
    JSON ESCAPING
    ================================================================
*/

std::string jsonEscape(
    const std::string& value
)
{
    std::string result;

    result.reserve(
        value.size() + 16
    );

    for(
        const char c :
        value
    )
    {
        switch(c)
        {
            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\b':
                result += "\\b";
                break;

            case '\f':
                result += "\\f";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:

                if(
                    static_cast<unsigned char>(c)
                    < 0x20
                )
                {
                    continue;
                }

                result += c;
                break;
        }
    }

    return result;
}


} // namespace discord_internal
