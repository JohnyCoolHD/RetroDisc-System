#include "registry_internal.hpp"
#include "registry.hpp"
#include "context.hpp"
#include "runtime.hpp"
#include "prefix_sanitize.hpp"
#include "../filesystem/filesystem_internal.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <pwd.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>


namespace
{


std::filesystem::path getHome()
{
    const char* home =
        std::getenv("HOME");


    if(
        home == nullptr ||
        *home == '\0'
    )
    {
        return {};
    }


    return std::filesystem::path(
        home
    );
}


std::string getUnixUsername()
{
    const auto uid =
        ::getuid();

    const auto* password =
        ::getpwuid(uid);

    if(
        password == nullptr ||
        password->pw_name == nullptr ||
        *password->pw_name == '\0'
    )
    {
        return {};
    }


    return std::string(
        password->pw_name
    );
}


std::filesystem::path findSteamRootForProton(
    const std::filesystem::path& proton
)
{
    const auto home = getHome();

    if(home.empty() || proton.empty())
    {
        return {};
    }

    const std::vector<std::filesystem::path> roots =
    {
        home / ".local" / "share" / "Steam",
        home / ".steam" / "root",
        home / ".steam" / "steam"
    };

    std::error_code protonError;

    const auto normalizedProton =
        std::filesystem::weakly_canonical(
            proton,
            protonError
        );

    if(protonError)
    {
        return {};
    }

    for(const auto& root : roots)
    {
        std::error_code ec;

        const auto normalizedRoot =
            std::filesystem::weakly_canonical(
                root,
                ec
            );

        if(ec)
        {
            continue;
        }

        const auto relative =
            normalizedProton.lexically_relative(
                normalizedRoot
            );

        if(
            !relative.empty() &&
            relative != "." &&
            relative.native().rfind("..", 0) != 0
        )
        {
            return normalizedRoot;
        }
    }

    return {};
}


class InitializationLock
{
public:

    explicit InitializationLock(
        const std::filesystem::path& path
    )
    {
        fd =
            ::open(
                path.c_str(),
                O_CREAT | O_RDWR | O_NOFOLLOW,
                0600
            );

        if(fd >= 0)
        {
            locked =
                (::flock(fd, LOCK_EX) == 0);
        }
    }

    ~InitializationLock()
    {
        if(fd >= 0)
        {
            if(locked)
            {
                ::flock(
                    fd,
                    LOCK_UN
                );
            }

            ::close(fd);
        }
    }

    bool isLocked() const
    {
        return locked;
    }

private:

    int fd = -1;
    bool locked = false;
};


/*
    ================================================================
    SANITIZE IDENTIFIER
    ================================================================
*/


std::string sanitizeIdentifier(
    const std::string& raw
)
{
    std::string result;
    result.reserve(
        raw.size()
    );


    for(const char c : raw)
    {
        if(
            c == '/' ||
            c == '\\' ||
            c == '\0'
        )
        {
            result += '_';
        }
        else
        {
            result += c;
        }
    }


    const auto start =
        result.find_first_not_of(
            " \t\r\n"
        );

    const auto end =
        result.find_last_not_of(
            " \t\r\n"
        );


    if(start == std::string::npos)
    {
        return "default";
    }


    result =
        result.substr(
            start,
            end - start + 1
        );


    if(
        result.empty() ||
        result == "." ||
        result == ".."
    )
    {
        return "default";
    }


    return result;
}


/*
    ================================================================
    RESOLVE PROTON VERSION IDENTIFIER
    ================================================================
*/


std::string resolveProtonVersionIdentifier(
    const std::filesystem::path& protonBinary
)
{
    if(protonBinary.empty())
    {
        return "default";
    }


    const auto directoryName =
        protonBinary
            .parent_path()
            .filename()
            .string();


    return sanitizeIdentifier(
        directoryName
    );
}


/*
    ================================================================
    RESOLVE WINE VERSION IDENTIFIER
    ================================================================
*/


std::string resolveWineVersionIdentifier()
{
    std::array<char, 256> buffer{};
    std::string output;


    FILE* pipe =
        popen(
            "wine --version 2>/dev/null",
            "r"
        );


    if(pipe == nullptr)
    {
        return "default";
    }


    while(
        fgets(
            buffer.data(),
            static_cast<int>(buffer.size()),
            pipe
        ) != nullptr
    )
    {
        output +=
            buffer.data();
    }


    const int status =
        pclose(pipe);


    if(
        status != 0 ||
        output.empty()
    )
    {
        return "default";
    }


    return sanitizeIdentifier(
        output
    );
}


/*
    ================================================================
    RESOLVE GLOBAL PREFIX DIRECTORY
    ================================================================
*/


std::filesystem::path resolveGlobalPrefixDirectory(
    const std::filesystem::path& home,
    const std::string& runtimeFolder,
    const std::string& versionIdentifier
)
{
    return
        home /
        ".RetroDisc" /
        "prefix" /
        runtimeFolder /
        versionIdentifier;
}


/*
    ================================================================
    COPY BUNDLED PREFIX AS DELTA
    ================================================================

    The global Wine prefix is the immutable LOWER layer.

    The bundled game prefix is compared against that lower layer.
    Only differences are copied into the persistent UPPER.

        global prefix
            +
        bundled differences
            =
        merged prefix

    The persistent game prefix therefore remains an overlay delta.
*/


bool filesEqual(
    const std::filesystem::path& left,
    const std::filesystem::path& right
)
{
    std::error_code ec;

    const auto leftStatus =
        std::filesystem::symlink_status(left, ec);

    if(ec || !std::filesystem::is_regular_file(leftStatus))
        return false;

    ec.clear();

    const auto rightStatus =
        std::filesystem::symlink_status(right, ec);

    if(ec || !std::filesystem::is_regular_file(rightStatus))
        return false;

    ec.clear();

    const auto leftSize =
        std::filesystem::file_size(left, ec);

    if(ec)
        return false;

    ec.clear();

    const auto rightSize =
        std::filesystem::file_size(right, ec);

    if(ec || leftSize != rightSize)
        return false;

    ec.clear();

    if(std::filesystem::equivalent(left, right, ec) && !ec)
        return true;

    std::ifstream a(left, std::ios::binary);
    std::ifstream b(right, std::ios::binary);

    if(!a.is_open() || !b.is_open())
        return false;

    std::array<char, 64 * 1024> ab{};
    std::array<char, 64 * 1024> bb{};

    for(;;)
    {
        a.read(
            ab.data(),
            static_cast<std::streamsize>(ab.size())
        );

        b.read(
            bb.data(),
            static_cast<std::streamsize>(bb.size())
        );

        if(a.gcount() != b.gcount())
            return false;

        if(!std::equal(
            ab.begin(),
            ab.begin() + a.gcount(),
            bb.begin()
        ))
        {
            return false;
        }

        if(a.eof() && b.eof())
            return true;

        if(!a && !a.eof())
            return false;

        if(!b && !b.eof())
            return false;
    }
}


bool isPersonalHomeSymlink(
    const std::filesystem::path& source,
    const std::filesystem::path& linkTarget
)
{
    std::filesystem::path target = linkTarget;

    if(target.is_relative())
        target = source.parent_path() / target;

    std::error_code ec;

    const auto canonical =
        std::filesystem::weakly_canonical(
            target,
            ec
        );

    if(ec)
        return false;

    const auto normalized =
        canonical.lexically_normal().string();

    return
        normalized == "/home" ||
        normalized.rfind(
            "/home/",
            0
        ) == 0;
}


/*
    ================================================================
    ENSURE WINE USER SYMLINK
    ================================================================

    Wine always uses RetroDisc as the canonical Windows user.

    The Unix username is linked to that directory so that paths
    generated by Wine using the Unix account name resolve to the
    same persistent Windows profile.

        drive_c/users/
        ├── RetroDisc/
        └── <unix-user> -> RetroDisc

    This is part of the persistent UPPER. It is therefore a
    game-specific delta and is never written to the global LOWER.
*/


bool ensureWineUserSymlink(
    const std::filesystem::path& persistentPrefix
)
{
    const auto unixUsername =
        getUnixUsername();

    if(unixUsername.empty())
    {
        std::cerr
            << "Could not determine Unix username."
            << std::endl;

        return false;
    }


    const auto usersDirectory =
        persistentPrefix /
        "drive_c" /
        "users";


    const auto canonicalUserDirectory =
        usersDirectory /
        "RetroDisc";


    const auto unixUserPath =
        usersDirectory /
        unixUsername;


    std::error_code ec;


    std::filesystem::create_directories(
        canonicalUserDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create RetroDisc Windows user directory:"
            << std::endl
            << "    "
            << canonicalUserDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    const auto status =
        std::filesystem::symlink_status(
            unixUserPath,
            ec
        );


    if(!ec)
    {
        if(std::filesystem::is_symlink(status))
        {
            const auto existingTarget =
                std::filesystem::read_symlink(
                    unixUserPath,
                    ec
                );

            if(
                !ec &&
                existingTarget == std::filesystem::path(
                    "RetroDisc"
                )
            )
            {
                return true;
            }


            if(ec)
            {
                std::cerr
                    << "Could not inspect existing Wine user symlink:"
                    << std::endl
                    << "    "
                    << unixUserPath
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }


            std::filesystem::remove(
                unixUserPath,
                ec
            );

            if(ec)
            {
                std::cerr
                    << "Could not replace existing Wine user symlink:"
                    << std::endl
                    << "    "
                    << unixUserPath
                    << std::endl
                    << "    "
                    << ec.message()
                    << std::endl;

                return false;
            }
        }
        else
        {
            /*
                Never delete a real user directory/file just to
                create our compatibility symlink.
            */

            std::cerr
                << "Wine Unix-user path already exists and is not"
                << " a symlink:"
                << std::endl
                << "    "
                << unixUserPath
                << std::endl;

            return false;
        }
    }
    else if(
        ec != std::errc::no_such_file_or_directory
    )
    {
        std::cerr
            << "Could not inspect Wine Unix-user path:"
            << std::endl
            << "    "
            << unixUserPath
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    ec.clear();


    std::filesystem::create_symlink(
        "RetroDisc",
        unixUserPath,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create Wine Unix-user compatibility symlink:"
            << std::endl
            << "    "
            << unixUserPath
            << " -> RetroDisc"
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::cout
        << "Wine user compatibility symlink:"
        << std::endl
        << "    "
        << unixUserPath
        << " -> RetroDisc"
        << std::endl;


    return true;
}


bool compactPersistentPrefix(
    const std::filesystem::path& upper,
    const std::filesystem::path& base
)
{
    std::error_code ec;

    if(!std::filesystem::is_directory(upper, ec))
        return true;

    const auto root =
        upper.lexically_normal();

    std::function<bool(
        const std::filesystem::path&
    )> compact =
        [&](const std::filesystem::path& directory) -> bool
        {
            std::vector<std::filesystem::path> entries;

            std::error_code iteratorError;

            std::filesystem::directory_iterator iterator(
                directory,
                std::filesystem::directory_options::
                    skip_permission_denied,
                iteratorError
            );

            if(iteratorError)
            {
                std::cerr
                    << "Could not list persistent prefix directory:"
                    << std::endl
                    << "    "
                    << directory
                    << std::endl
                    << "    "
                    << iteratorError.message()
                    << std::endl;

                return false;
            }

            for(const auto& entry : iterator)
                entries.push_back(entry.path());

            for(const auto& entry : entries)
            {
                const auto relative =
                    entry.lexically_relative(root);

                const auto baseEntry =
                    base / relative;

                std::error_code entryError;

                const auto status =
                    std::filesystem::symlink_status(
                        entry,
                        entryError
                    );

                if(entryError)
                {
                    std::cerr
                        << "Could not inspect persistent prefix entry:"
                        << std::endl
                        << "    "
                        << entry
                        << std::endl
                        << "    "
                        << entryError.message()
                        << std::endl;

                    return false;
                }

                if(std::filesystem::is_directory(status))
                {
                    if(!compact(entry))
                        return false;

                    continue;
                }

                if(std::filesystem::is_regular_file(status))
                {
                    if(filesEqual(entry, baseEntry))
                    {
                        std::filesystem::remove(
                            entry,
                            entryError
                        );

                        if(entryError)
                        {
                            std::cerr
                                << "Could not remove redundant persistent"
                                << " prefix file:"
                                << std::endl
                                << "    "
                                << entry
                                << std::endl
                                << "    "
                                << entryError.message()
                                << std::endl;

                            return false;
                        }
                    }

                    continue;
                }

                if(std::filesystem::is_symlink(status))
                {
                    std::error_code baseError;

                    const auto baseStatus =
                        std::filesystem::symlink_status(
                            baseEntry,
                            baseError
                        );

                    bool same = false;

                    if(
                        !baseError &&
                        std::filesystem::is_symlink(
                            baseStatus
                        )
                    )
                    {
                        std::error_code targetError;
                        std::error_code existingTargetError;

                        const auto target =
                            std::filesystem::read_symlink(
                                entry,
                                targetError
                            );

                        const auto existingTarget =
                            std::filesystem::read_symlink(
                                baseEntry,
                                existingTargetError
                            );

                        same =
                            !targetError &&
                            !existingTargetError &&
                            target == existingTarget;
                    }

                    if(same)
                    {
                        std::filesystem::remove(
                            entry,
                            entryError
                        );

                        if(entryError)
                        {
                            std::cerr
                                << "Could not remove redundant persistent"
                                << " prefix symlink:"
                                << std::endl
                                << "    "
                                << entry
                                << std::endl
                                << "    "
                                << entryError.message()
                                << std::endl;

                            return false;
                        }
                    }

                    continue;
                }

                /*
                    Overlay whiteouts/unknown entries are intentionally
                    retained.
                */
            }

            if(directory != root)
            {
                std::error_code checkError;

                if(std::filesystem::is_empty(
                    directory,
                    checkError
                ))
                {
                    std::filesystem::remove(
                        directory,
                        checkError
                    );

                    if(checkError)
                    {
                        std::cerr
                            << "Could not remove now-empty persistent"
                            << " prefix directory:"
                            << std::endl
                            << "    "
                            << directory
                            << std::endl
                            << "    "
                            << checkError.message()
                            << std::endl;

                        return false;
                    }
                }
            }

            return true;
        };

    return compact(root);
}


bool copyBundledPrefix(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    const std::filesystem::path& base
)
{
    std::error_code ec;


    const auto sourceDriveC =
        source / "drive_c";

    const auto sourceDosDevices =
        source / "dosdevices";

    const auto sourceSystemReg =
        source / "system.reg";

    const auto sourceUserReg =
        source / "user.reg";


    if(
        !directoryExists(sourceDriveC) ||
        !directoryExists(sourceDosDevices) ||
        !regularFileExists(sourceSystemReg) ||
        !regularFileExists(sourceUserReg)
    )
    {
        std::cerr
            << "Bundled Wine prefix is invalid or incomplete:"
            << std::endl
            << "    "
            << source
            << std::endl;

        return false;
    }


    const auto baseRoot =
        (base / "pfx").lexically_normal();


    if(
        !directoryExists(
            baseRoot / "drive_c"
        ) ||
        !directoryExists(
            baseRoot / "dosdevices"
        ) ||
        !regularFileExists(
            baseRoot / "system.reg"
        ) ||
        !regularFileExists(
            baseRoot / "user.reg"
        )
    )
    {
        std::cerr
            << "Selected global base prefix is invalid or incomplete:"
            << std::endl
            << "    "
            << baseRoot
            << std::endl;

        return false;
    }


    const auto destinationStatus =
        std::filesystem::symlink_status(
            destination,
            ec
        );


    if(
        !ec &&
        (
            std::filesystem::exists(
                destinationStatus
            ) ||
            std::filesystem::is_symlink(
                destinationStatus
            )
        )
    )
    {
        std::cerr
            << "Wine prefix delta destination already exists:"
            << std::endl
            << "    "
            << destination
            << std::endl;

        return false;
    }


    if(ec != std::errc::no_such_file_or_directory)
    {
        std::cerr
            << "Could not inspect Wine prefix delta destination:"
            << std::endl
            << "    "
            << destination
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    ec.clear();


    std::filesystem::create_directories(
        destination,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create Wine prefix delta destination:"
            << std::endl
            << "    "
            << destination
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::cout
        << "Creating bundled Wine prefix delta:"
        << std::endl
        << "    Source: "
        << source
        << std::endl
        << "    Base:   "
        << base
        << std::endl
        << "    Base root:"
        << std::endl
        << "        "
        << baseRoot
        << std::endl
        << "    Upper:  "
        << destination
        << std::endl;


    const auto sourceRoot =
        source.lexically_normal();


    std::filesystem::recursive_directory_iterator iterator(
        source,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not iterate bundled Wine prefix:"
            << std::endl
            << "    "
            << source
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    const auto end =
        std::filesystem::recursive_directory_iterator();


    while(iterator != end)
    {
        const auto sourcePath =
            iterator->path();


        const auto relative =
            sourcePath.lexically_relative(
                sourceRoot
            );


        const auto target =
            destination / relative;


        const auto basePath =
            baseRoot / relative;


        std::error_code entryError;


        const auto status =
            std::filesystem::symlink_status(
                sourcePath,
                entryError
            );


        if(entryError)
        {
            std::cerr
                << "Could not inspect bundled Wine prefix entry:"
                << std::endl
                << "    "
                << sourcePath
                << std::endl
                << "    "
                << entryError.message()
                << std::endl;

            return false;
        }


        if(std::filesystem::is_symlink(status))
        {
            const auto linkTarget =
                std::filesystem::read_symlink(
                    sourcePath,
                    entryError
                );


            if(entryError)
            {
                std::cerr
                    << "Could not read bundled Wine prefix symlink:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            if(isPersonalHomeSymlink(
                   sourcePath,
                   linkTarget
               ))
            {
                ++iterator;
                continue;
            }


            bool same = false;


            const auto baseStatus =
                std::filesystem::symlink_status(
                    basePath,
                    entryError
                );


            if(
                !entryError &&
                std::filesystem::is_symlink(
                    baseStatus
                )
            )
            {
                std::error_code targetError;


                const auto existingTarget =
                    std::filesystem::read_symlink(
                        basePath,
                        targetError
                    );


                same =
                    !targetError &&
                    existingTarget == linkTarget;
            }


            if(!same)
            {
                std::filesystem::create_directories(
                    target.parent_path(),
                    entryError
                );


                if(entryError)
                {
                    std::cerr
                        << "Could not create parent directory for"
                        << " bundled Wine prefix symlink:"
                        << std::endl
                        << "    "
                        << target.parent_path()
                        << std::endl
                        << "    "
                        << entryError.message()
                        << std::endl;

                    return false;
                }


                std::filesystem::create_symlink(
                    linkTarget,
                    target,
                    entryError
                );


                if(entryError)
                {
                    std::cerr
                        << "Could not create bundled Wine prefix symlink:"
                        << std::endl
                        << "    "
                        << target
                        << std::endl
                        << "    "
                        << entryError.message()
                        << std::endl;

                    return false;
                }
            }


            ++iterator;
            continue;
        }


        if(std::filesystem::is_directory(status))
        {
            const auto baseStatus =
                std::filesystem::symlink_status(
                    basePath,
                    entryError
                );


            if(
                !entryError &&
                std::filesystem::is_directory(
                    baseStatus
                )
            )
            {
                ++iterator;
                continue;
            }


            std::filesystem::create_directories(
                target,
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create bundled Wine prefix"
                    << " directory:"
                    << std::endl
                    << "    "
                    << target
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            ++iterator;
            continue;
        }


        if(std::filesystem::is_regular_file(status))
        {
            if(filesEqual(
                   sourcePath,
                   basePath
               ))
            {
                ++iterator;
                continue;
            }


            std::filesystem::create_directories(
                target.parent_path(),
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not create parent directory for"
                    << " bundled Wine prefix file:"
                    << std::endl
                    << "    "
                    << target.parent_path()
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            std::filesystem::copy_file(
                sourcePath,
                target,
                std::filesystem::copy_options::none,
                entryError
            );


            if(entryError)
            {
                std::cerr
                    << "Could not copy bundled Wine prefix file:"
                    << std::endl
                    << "    "
                    << sourcePath
                    << std::endl
                    << "to:"
                    << std::endl
                    << "    "
                    << target
                    << std::endl
                    << "    "
                    << entryError.message()
                    << std::endl;

                return false;
            }


            ++iterator;
            continue;
        }


        std::cerr
            << "Unsupported bundled Wine prefix entry:"
            << std::endl
            << "    "
            << sourcePath
            << std::endl;

        return false;
    }


    std::cout
        << "Bundled Wine prefix delta created successfully."
        << std::endl;


    return true;
}


/*
    ================================================================
    INITIALIZE GLOBAL PREFIX
    ================================================================
*/


bool initializeGlobalPrefix(
    Context& ctx
)
{
    const auto home =
        getHome();


    if(home.empty())
    {
        std::cerr
            << "Could not determine HOME directory."
            << std::endl;

        return false;
    }


    const std::string runtimeFolder =
        ctx.runtime.empty()
            ? std::string("wine")
            : ctx.runtime;

    const bool useProton =
        (runtimeFolder == "proton");


    std::filesystem::path proton;

    if(useProton)
    {
        proton =
            resolveProton(ctx);

        if(proton.empty())
        {
            std::cerr
                << "Could not find Proton for global base prefix."
                << std::endl;

            return false;
        }

        std::error_code canonicalError;

        const auto canonicalProton =
            std::filesystem::weakly_canonical(
                proton,
                canonicalError
            );

        ctx.resolvedProtonPath =
            canonicalError
                ? proton
                : canonicalProton;
    }


    const std::string versionIdentifier =
        useProton
            ? resolveProtonVersionIdentifier(
                ctx.resolvedProtonPath
            )
            : resolveWineVersionIdentifier();


    ctx.globalPrefixDirectory =
        resolveGlobalPrefixDirectory(
            home,
            runtimeFolder,
            versionIdentifier
        );


    const auto compatDataDirectory =
        ctx.globalPrefixDirectory;


    const auto lockPath =
        compatDataDirectory /
        ".init.lock";

    std::error_code ec;


    const bool globalPrefixExisted =
        std::filesystem::exists(
            ctx.globalPrefixDirectory,
            ec
        );

    if(ec)
    {
        std::cerr
            << "Could not inspect global base prefix:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::filesystem::create_directories(
        compatDataDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create global base prefix directory:"
            << std::endl
            << "    "
            << compatDataDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    InitializationLock lock(
        lockPath
    );

    if(!lock.isLocked())
    {
        std::cerr
            << "Could not lock global base prefix initialization:"
            << std::endl
            << "    "
            << lockPath
            << std::endl;

        return false;
    }


    if(globalPrefixExisted)
    {
        const auto prefixStatus =
            std::filesystem::symlink_status(
                ctx.globalPrefixDirectory,
                ec
            );

        if(ec)
        {
            std::cerr
                << "Could not inspect global base prefix:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        if(!std::filesystem::is_directory(
            prefixStatus
        ))
        {
            std::cerr
                << "Global base prefix path is not a real directory:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl;

            return false;
        }


        const bool prefixValid =
            useProton
                ? prefixLooksValid(
                    ctx.globalPrefixDirectory
                )
                : winePrefixLooksValid(
                    ctx.globalPrefixDirectory
                );


        if(!prefixValid)
        {
            std::cerr
                << "Global base prefix exists but is incomplete:"
                << std::endl
                << "    "
                << ctx.globalPrefixDirectory
                << std::endl;

            if(useProton)
            {
                std::cerr
                    << "Required Proton compat-data files:"
                    << std::endl
                    << "    "
                    << compatDataDirectory / "version"
                    << std::endl
                    << "    "
                    << compatDataDirectory / "tracked_files"
                    << std::endl;
            }
            else
            {
                std::cerr
                    << "Required Wine prefix files:"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "drive_c"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "dosdevices"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "system.reg"
                    << std::endl
                    << "    "
                    << ctx.globalPrefixDirectory / "pfx" / "user.reg"
                    << std::endl;
            }

            std::cerr
                << "Refusing to overwrite the existing base prefix."
                << std::endl;

            return false;
        }


        if(useProton)
        {
            const auto versionFile =
                compatDataDirectory /
                "version";

            const auto trackedFiles =
                compatDataDirectory /
                "tracked_files";


            const auto versionStatus =
                std::filesystem::symlink_status(
                    versionFile,
                    ec
                );

            if(
                ec ||
                !std::filesystem::is_regular_file(
                    versionStatus
                )
            )
            {
                std::cerr
                    << "Global Proton compat-data version file is invalid:"
                    << std::endl
                    << "    "
                    << versionFile
                    << std::endl;

                return false;
            }


            ec.clear();


            const auto trackedStatus =
                std::filesystem::symlink_status(
                    trackedFiles,
                    ec
                );

            if(
                ec ||
                !std::filesystem::is_regular_file(
                    trackedStatus
                )
            )
            {
                std::cerr
                    << "Global Proton compat-data tracked_files is invalid:"
                    << std::endl
                    << "    "
                    << trackedFiles
                    << std::endl;

                return false;
            }
        }


        std::cout
            << "Global base prefix found ("
            << runtimeFolder
            << " / "
            << versionIdentifier
            << "):"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl;

        return true;
    }


    std::filesystem::create_directories(
        compatDataDirectory,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create global base prefix:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    std::cout
        << "Initializing global base prefix ("
        << runtimeFolder
        << " / "
        << versionIdentifier
        << "):"
        << std::endl
        << "    "
        << ctx.globalPrefixDirectory
        << std::endl;


    if(useProton)
    {
        const auto steamInstallPath =
            findSteamRootForProton(
                ctx.resolvedProtonPath
            );


        if(steamInstallPath.empty())
        {
            std::cerr
                << "Could not determine Steam root for Proton:"
                << std::endl
                << "    "
                << ctx.resolvedProtonPath
                << std::endl;

            return false;
        }


        std::cout
            << "Steam install path:"
            << std::endl
            << "    "
            << steamInstallPath
            << std::endl;


        std::ostringstream command;


        command
            << "STEAM_COMPAT_CLIENT_INSTALL_PATH="
            << shellQuote(
                steamInstallPath.string()
            )
            << " "
            << "STEAM_COMPAT_DATA_PATH="
            << shellQuote(
                compatDataDirectory.string()
            )
            << " "
            << "WINEPREFIX="
            << shellQuote(
                (compatDataDirectory / "pfx").string()
            )
            << " "
            << shellQuote(
                ctx.resolvedProtonPath.string()
            )
            << " run "
            << shellQuote(
                "wineboot"
            );


        if(!runCommand(
            command.str(),
            true
        ))
        {
            std::cerr
                << "Could not initialize global base prefix."
                << std::endl;

            return false;
        }
    }
    else
    {
        std::ostringstream command;


        command
            << "WINEPREFIX="
            << shellQuote(
                (
                    ctx.globalPrefixDirectory /
                    "pfx"
                ).string()
            )
            << " wineboot";


        if(!runCommand(
            command.str(),
            true
        ))
        {
            std::cerr
                << "Could not initialize global base prefix."
                << std::endl;

            return false;
        }
    }


    {
        const std::filesystem::path winePrefix =
            ctx.globalPrefixDirectory /
            "pfx";


        const std::string waitCommand =
            "WINEPREFIX=" +
            shellQuote(
                winePrefix.string()
            ) +
            " wineserver -w";


        if(!runCommand(
            waitCommand,
            true
        ))
        {
            std::cerr
                << "Could not wait for Wine server after prefix initialization."
                << std::endl;

            return false;
        }
    }


    const bool prefixValid =
        useProton
            ? prefixLooksValid(
                ctx.globalPrefixDirectory
            )
            : winePrefixLooksValid(
                ctx.globalPrefixDirectory
            );


    if(!prefixValid)
    {
        std::cerr
            << "Wineboot completed, but the global base prefix is"
            << " incomplete:"
            << std::endl
            << "    "
            << ctx.globalPrefixDirectory
            << std::endl;

        return false;
    }


    if(useProton)
    {
        const auto versionFile =
            compatDataDirectory /
            "version";

        const auto trackedFiles =
            compatDataDirectory /
            "tracked_files";


        ec.clear();


        if(
            !std::filesystem::is_regular_file(
                versionFile,
                ec
            )
        )
        {
            std::cerr
                << "Proton did not create a valid version file:"
                << std::endl
                << "    "
                << versionFile
                << std::endl;

            return false;
        }


        ec.clear();


        if(
            !std::filesystem::is_regular_file(
                trackedFiles,
                ec
            )
        )
        {
            std::cerr
                << "Proton did not create a valid tracked_files file:"
                << std::endl
                << "    "
                << trackedFiles
                << std::endl;

            return false;
        }
    }


    std::cout
        << "Global base prefix initialized successfully:"
        << std::endl
        << "    "
        << ctx.globalPrefixDirectory
        << std::endl;


    return true;
}


/*
    ================================================================
    PREPARE GAME PREFIX
    ================================================================
*/

bool prepareGamePrefix(Context& ctx)
{
    if(!initializeGlobalPrefix(ctx))
    {
        return false;
    }


    if(ctx.gameDirectory.empty())
    {
        std::cerr
            << "Game directory is empty."
            << std::endl;

        return false;
    }


    /*
        Persistent game data:

            <datapath>/
            └── prefix/
                └── pfx/
                    └── only persistent changes

        The complete Wine/Proton prefix lives in:

            ~/.RetroDisc/prefix/wine/<version>/pfx
            ~/.RetroDisc/prefix/proton/<version>/pfx

        and is used as the overlay lower directory.
    */

    const auto persistentGameDirectory =
        ctx.dataPath.empty()
            ? getHome() /
              "Games" /
              "RetroDisc" /
              ctx.gameId
            : ctx.dataPath;


    ctx.prefixLowerDirectory =
        ctx.globalPrefixDirectory;


    ctx.prefixOverlayDirectory =
        persistentGameDirectory /
        "prefix";


    ctx.prefixWorkDirectory =
        persistentGameDirectory /
        ".prefix_work";


    /*
        The persistent prefix upper must itself remain
        a delta. Never copy the complete global prefix
        into this directory.
    */

    if(!sanitizePersistentPrefixDirectory(
        ctx.prefixOverlayDirectory,
        ctx.prefixLowerDirectory
    ))
    {  
        return false;
    }


    const auto temporaryPrefixDirectory =
        std::filesystem::path("/tmp") /
        (
            ctx.gameId +
            "-" +
            std::to_string(getpid())
        );


    ctx.prefixMergedDirectory =
        temporaryPrefixDirectory /
        "merged_prefix";


    ctx.prefixMergedPfxDirectory =
        ctx.prefixMergedDirectory /
        "pfx";


    std::error_code ec;


    std::filesystem::create_directories(
        temporaryPrefixDirectory,
        ec
    );


    if(ec)
    {
        std::cerr
            << "Could not create temporary prefix directory:"
            << std::endl
            << "    "
            << temporaryPrefixDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    /*
        Check whether the persistent upper already exists.

        IMPORTANT:

        We do NOT create a complete prefix here.

        If it does not exist, we only create the upper
        directory itself. The complete pfx/ comes from
        the lower directory through fuse-overlayfs.
    */

    ec.clear();

    const bool persistentPrefixExists =
        std::filesystem::exists(
            ctx.prefixOverlayDirectory,
            ec
        );


    if(ec)
    {
        std::cerr
            << "Could not inspect persistent prefix upper:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }


    const bool useWine =
        ctx.runtime != "proton";


    if(persistentPrefixExists)
    {
        std::cout
            << "Persistent game prefix upper found:"
            << std::endl
            << "    "
            << ctx.prefixOverlayDirectory
            << std::endl;


        /*
            Remove files from the persistent upper which
            are byte-identical to the lower.

            This keeps the game prefix a real delta.
        */

        if(!compactPersistentPrefix(
            ctx.prefixOverlayDirectory,
            ctx.prefixLowerDirectory
        ))
        {
            std::cerr
                << "Could not compact persistent prefix upper."
                << std::endl;

            return false;
        }
    }
    else
    {
        /*
            IMPORTANT:

            Do NOT copy the global prefix here.

            Do NOT call copyCompletePrefix().

            The persistent prefix starts empty and fuse-overlayfs
            exposes the complete global prefix through lowerdir.
        */

        std::filesystem::create_directories(
            ctx.prefixOverlayDirectory,
            ec
        );


        if(ec)
        {
            std::cerr
                << "Could not create persistent prefix upper:"
                << std::endl
                << "    "
                << ctx.prefixOverlayDirectory
                << std::endl
                << "    "
                << ec.message()
                << std::endl;

            return false;
        }


        if(useWine)
        {
            /*
                A bundled Wine prefix is a special case.

                The bundled prefix is a direct Wine prefix:

                    bundled/pfx/
                        drive_c/
                        dosdevices/
                        system.reg
                        user.reg

                We create only the differences against
                the global Wine prefix in the persistent
                upper.
            */

            const auto executablePath =
                std::filesystem::path(
                    ctx.executable
                );


            const auto executableParent =
                executablePath.parent_path();


            const auto bundledPrefix =
                executableParent.empty()
                    ? ctx.root / "pfx"
                    : ctx.root /
                      executableParent /
                      "pfx";


            std::filesystem::path bundledPrefixToCopy =
                bundledPrefix;


            if(
                !std::filesystem::is_directory(
                    bundledPrefixToCopy
                ) &&
                bundledPrefixToCopy !=
                    ctx.root / "pfx" &&
                std::filesystem::is_directory(
                    ctx.root / "pfx"
                )
            )
            {
                bundledPrefixToCopy =
                    ctx.root / "pfx";
            }


            if(std::filesystem::is_directory(
                bundledPrefixToCopy
            ))
            {
                std::cout
                    << "Bundled game Wine prefix found:"
                    << std::endl
                    << "    "
                    << bundledPrefixToCopy
                    << std::endl;


                /*
                    IMPORTANT:

                    copyBundledPrefix() creates a DELTA
                    against the global prefix.

                    It must NOT create a complete copy.
                */

                if(!copyBundledPrefix(
                    bundledPrefixToCopy,
                    ctx.prefixOverlayDirectory / "pfx",
                    ctx.globalPrefixDirectory
                ))
                {
                    std::cerr
                        << "Could not create bundled Wine prefix delta."
                        << std::endl;

                    return false;
                }
            }
            else
            {
                /*
                    No bundled prefix.

                    Only create the upper root.

                    We intentionally do NOT create/copy
                    prefix/pfx here. The lower prefix provides
                    that through the overlay.
                */

                std::cout
                    << "No bundled Wine prefix found."
                    << std::endl
                    << "Creating empty persistent Wine prefix upper."
                    << std::endl;
            }
        }
        else
        {
            std::cout
                << "Creating empty persistent Proton prefix upper."
                << std::endl;
        }
    }


    /*
        Wine needs the compatibility symlink inside the
        persistent upper.

        Do this AFTER compaction so the intentional symlink
        is not removed by the delta cleanup.
    */

    if(useWine)
    {
        const auto persistentPfx =
            ctx.prefixOverlayDirectory /
            "pfx";


        /*
            For an empty upper this creates:

                prefix/pfx/drive_c/users/RetroDisc

            without copying the global prefix.
        */

        if(!ensureWineUserSymlink(
            persistentPfx
        ))
        {
            return false;
        }
    }


    /*
        Do not copy or clone the global prefix here.

        The only thing that must exist before mounting is:

            <datapath>/prefix/

        plus, where required, actual persistent changes
        such as bundled-prefix differences or the Wine
        RetroDisc user symlink.
    */

    if(!sanitizePersistentPrefixDirectory(
        ctx.prefixOverlayDirectory,
        ctx.prefixLowerDirectory
    ))
    {
        return false;
    }

    /*
        Recreate the complete DIRECTORY STRUCTURE of the lower prefix
        inside the persistent upper after sanitizing it.

        Only directories are replicated.

        No regular files are copied.
        No symlinks are copied.

        The lower prefix therefore remains the source for all runtime
        files, while the persistent upper contains the complete
        directory structure.

        This is deliberately done AFTER sanitizePersistentPrefixDirectory().
        The sanitizer can therefore remove pfx/drive_c/windows first,
        and the directory structure is recreated afterwards.
    */

    std::cout
        << "Replicating persistent prefix directory structure from lower:"
        << std::endl
        << "    Lower: "
        << ctx.prefixLowerDirectory
        << std::endl
        << "    Upper: "
        << ctx.prefixOverlayDirectory
        << std::endl;


    if(!replicateDirectoryStructure(
        ctx.prefixLowerDirectory,
        ctx.prefixOverlayDirectory
    ))
    {
        std::cerr
            << "Could not replicate persistent prefix directory structure."
            << std::endl;

        return false;
    }


    std::cout
        << "Persistent prefix directory structure replicated successfully."
        << std::endl;

    std::cout
        << "Mounting prefix overlay..."
        << std::endl;


    if(!mountPrefixOverlay(ctx))
    {
        return false;
    }


    if(
        ctx.prefixMergedPfxDirectory.empty() ||
        !std::filesystem::exists(
            ctx.prefixMergedPfxDirectory
        )
    )
    {
        std::cerr
            << "Merged prefix directory does not exist:"
            << std::endl
            << "    "
            << ctx.prefixMergedPfxDirectory
            << std::endl;


        unmountPath(
            ctx.prefixMergedDirectory
        );


        ctx.prefixOverlayMounted =
            false;


        return false;
    }


    std::cout
        << "Wine/Proton merged prefix ready:"
        << std::endl
        << "    "
        << ctx.prefixMergedPfxDirectory
        << std::endl;


    return true;
}

}


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