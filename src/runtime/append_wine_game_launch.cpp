#include "runtime_internal.hpp"
#include "context.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    WINE GAME LAUNCH
    ================================================================
*/

void appendWineGameLaunch(
    std::ostringstream& command,
    const Context& ctx
)
{
    const auto executable =
        (
            ctx.mergedDirectory /
            ctx.executable
        ).string();

    if(ctx.wine.display.virtualDesktop.enabled)
    {
        const int width =
            ctx.wine.display.virtualDesktop.width > 0
                ? ctx.wine.display.virtualDesktop.width
                : 640;

        const int height =
            ctx.wine.display.virtualDesktop.height > 0
                ? ctx.wine.display.virtualDesktop.height
                : 480;

        const std::string desktop =
            "Default," +
            std::to_string(width) +
            "x" +
            std::to_string(height);

        command
            << "wine explorer "
            << shellQuote(
                "/desktop=" + desktop
            )
            << " "
            << shellQuote(executable);
    }
    else
    {
        command
            << "wine "
            << shellQuote(executable);
    }

    for(const auto& argument : ctx.arguments)
    {
        command
            << " "
            << shellQuote(argument);
    }
}


} // namespace runtime_internal
