#include "main_internal.hpp"
#include "context.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>


namespace main_internal
{


/*
    ================================================================
    COMMAND LINE
    ================================================================
*/

bool parseCommandLine(
    int argc,
    char* argv[],
    Context& ctx
)
{
    for(int i = 1; i < argc; ++i)
    {
        if(argv[i] == nullptr)
        {
            continue;
        }

        const std::string argument =
            argv[i];

        /*
            --------------------------------------------------------
            --datapath
            --------------------------------------------------------
        */

        if(argument == "--datapath")
        {
            if(i + 1 >= argc)
            {
                std::cerr
                    << "--datapath requires a directory path."
                    << std::endl;

                return false;
            }

            if(argv[i + 1] == nullptr)
            {
                std::cerr
                    << "--datapath requires a directory path."
                    << std::endl;

                return false;
            }

            const std::string path =
                argv[++i];

            if(path.empty())
            {
                std::cerr
                    << "--datapath requires a directory path."
                    << std::endl;

                return false;
            }

            ctx.dataPath =
                std::filesystem::path(
                    path
                );

            /*
                Relative datapaths are resolved against the
                current working directory.
            */

            if(ctx.dataPath.is_relative())
            {
                try
                {
                    ctx.dataPath =
                        std::filesystem::absolute(
                            ctx.dataPath
                        );
                }
                catch(const std::exception& e)
                {
                    std::cerr
                        << "Could not resolve --datapath:"
                        << std::endl
                        << "    "
                        << e.what()
                        << std::endl;

                    return false;
                }
            }

            ctx.dataPath =
                ctx.dataPath.lexically_normal();

            continue;
        }

        /*
            --------------------------------------------------------
            UNKNOWN OPTION
            --------------------------------------------------------
        */

        if(
            argument.size() >= 2 &&
            argument[0] == '-' &&
            argument[1] == '-'
        )
        {
            std::cerr
                << "Unknown option:"
                << std::endl
                << "    "
                << argument
                << std::endl;

            return false;
        }
    }

    return true;
}


} // namespace main_internal
