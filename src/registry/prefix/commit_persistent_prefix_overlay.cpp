#include "prefix_internal.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>


namespace prefix_internal
{

namespace
{

bool sameSymlinkTarget(
    const std::filesystem::path& left,
    const std::filesystem::path& right
)
{
    std::error_code ecLeft;
    std::error_code ecRight;

    const auto leftStatus =
        std::filesystem::symlink_status(left, ecLeft);
    const auto rightStatus =
        std::filesystem::symlink_status(right, ecRight);

    if(
        ecLeft ||
        ecRight ||
        !std::filesystem::is_symlink(leftStatus) ||
        !std::filesystem::is_symlink(rightStatus)
    )
    {
        return false;
    }

    std::error_code leftTargetError;
    std::error_code rightTargetError;

    const auto leftTarget =
        std::filesystem::read_symlink(left, leftTargetError);
    const auto rightTarget =
        std::filesystem::read_symlink(right, rightTargetError);

    return
        !leftTargetError &&
        !rightTargetError &&
        leftTarget == rightTarget;
}


bool isWhiteoutName(
    const std::filesystem::path& path
)
{
    const auto name = path.filename().string();
    return name.rfind(".wh.", 0) == 0;
}


std::filesystem::path whiteoutTarget(
    const std::filesystem::path& whiteout
)
{
    const auto name = whiteout.filename().string();

    if(name == ".wh..wh..opq")
        return {};

    if(name.rfind(".wh.", 0) != 0)
        return {};

    return
        whiteout.parent_path() /
        name.substr(4);
}


void removeWhiteoutFor(
    const std::filesystem::path& path
)
{
    const auto whiteout =
        path.parent_path() /
        (".wh." + path.filename().string());

    std::error_code ec;
    std::filesystem::remove(whiteout, ec);
}


bool copyEntry(
    const std::filesystem::path& source,
    const std::filesystem::path& destination
)
{
    std::error_code ec;

    std::filesystem::create_directories(
        destination.parent_path(),
        ec
    );

    if(ec)
        return false;

    const auto status =
        std::filesystem::symlink_status(source, ec);

    if(ec)
        return false;

    std::filesystem::remove_all(destination, ec);
    if(ec)
        return false;

    if(std::filesystem::is_symlink(status))
    {
        std::error_code targetError;
        const auto target =
            std::filesystem::read_symlink(source, targetError);

        if(targetError)
            return false;

        std::filesystem::create_symlink(
            target,
            destination,
            targetError
        );

        return !targetError;
    }

    if(std::filesystem::is_regular_file(status))
    {
        std::filesystem::copy_file(
            source,
            destination,
            std::filesystem::copy_options::overwrite_existing,
            ec
        );

        return !ec;
    }

    /*
        The runtime upper is deliberately configured to use
        .wh.* whiteout files instead of device-node whiteouts.
        Other special files are not expected in a Wine/Proton
        prefix. Refuse to silently lose one if it appears.
    */
    return false;
}


bool commitDirectory(
    const std::filesystem::path& runtimeDirectory,
    const std::filesystem::path& persistentDirectory,
    const std::filesystem::path& lowerDirectory
)
{
    std::error_code ec;

    if(!std::filesystem::is_directory(
        runtimeDirectory,
        ec
    ))
    {
        return true;
    }

    bool wroteAnything = false;

    std::filesystem::directory_iterator iterator(
        runtimeDirectory,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not list runtime prefix upper:"
            << std::endl
            << "    "
            << runtimeDirectory
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    for(const auto& entry : iterator)
    {
        const auto relative =
            entry.path().lexically_relative(runtimeDirectory);

        const auto destination =
            persistentDirectory / relative;

        const auto lowerEntry =
            lowerDirectory / relative;

        const auto status =
            std::filesystem::symlink_status(entry.path(), ec);

        if(ec)
            return false;

        if(std::filesystem::is_directory(status))
        {
            if(!commitDirectory(
                entry.path(),
                destination,
                lowerEntry
            ))
            {
                return false;
            }

            /*
                The directory itself is only metadata. Do not create
                an otherwise empty persistent directory. Child entries
                create it when they are actually persisted.
            */
            continue;
        }

        if(isWhiteoutName(entry.path()))
        {
            const auto hiddenTarget =
                whiteoutTarget(entry.path());

            if(!hiddenTarget.empty())
            {
                std::error_code removeError;
                std::filesystem::remove_all(
                    destination.parent_path() /
                    hiddenTarget.filename(),
                    removeError
                );

                if(removeError)
                {
                    std::cerr
                        << "Could not remove persistent file hidden by whiteout:"
                        << std::endl
                        << "    "
                        << (destination.parent_path() / hiddenTarget.filename())
                        << std::endl
                        << "    "
                        << removeError.message()
                        << std::endl;

                    return false;
                }
            }

            if(!copyEntry(entry.path(), destination))
            {
                std::cerr
                    << "Could not persist prefix whiteout:"
                    << std::endl
                    << "    "
                    << entry.path()
                    << std::endl;

                return false;
            }

            wroteAnything = true;
            continue;
        }

        if(std::filesystem::is_regular_file(status))
        {
            /*
                fuse-overlayfs copies the complete lower file into its
                runtime upper even for metadata-only operations. Those
                files are byte-identical to either the persistent delta
                or the immutable base and must never be promoted into
                persistent storage.
            */
            if(filesEqual(entry.path(), lowerEntry))
            {
                removeWhiteoutFor(destination);
                continue;
            }

            if(filesEqual(entry.path(), destination))
            {
                removeWhiteoutFor(destination);
                continue;
            }

            removeWhiteoutFor(destination);

            if(!copyEntry(entry.path(), destination))
            {
                std::cerr
                    << "Could not persist changed prefix file:"
                    << std::endl
                    << "    "
                    << entry.path()
                    << std::endl
                    << " -> "
                    << destination
                    << std::endl;

                return false;
            }

            wroteAnything = true;
            continue;
        }

        if(std::filesystem::is_symlink(status))
        {
            if(sameSymlinkTarget(entry.path(), lowerEntry))
            {
                removeWhiteoutFor(destination);
                continue;
            }

            if(sameSymlinkTarget(entry.path(), destination))
            {
                removeWhiteoutFor(destination);
                continue;
            }

            removeWhiteoutFor(destination);

            if(!copyEntry(entry.path(), destination))
            {
                std::cerr
                    << "Could not persist changed prefix symlink:"
                    << std::endl
                    << "    "
                    << entry.path()
                    << std::endl;

                return false;
            }

            wroteAnything = true;
            continue;
        }

        std::cerr
            << "Unsupported entry in runtime prefix upper:"
            << std::endl
            << "    "
            << entry.path()
            << std::endl;

        return false;
    }

    return true;
}

} // namespace


bool commitPersistentPrefixOverlay(
    const std::filesystem::path& runtimeUpper,
    const std::filesystem::path& persistentUpper,
    const std::filesystem::path& lower
)
{
    std::error_code ec;

    if(!std::filesystem::is_directory(runtimeUpper, ec))
        return true;

    std::filesystem::create_directories(
        persistentUpper,
        ec
    );

    if(ec)
    {
        std::cerr
            << "Could not create persistent prefix upper:"
            << std::endl
            << "    "
            << persistentUpper
            << std::endl
            << "    "
            << ec.message()
            << std::endl;

        return false;
    }

    return commitDirectory(
        runtimeUpper,
        persistentUpper,
        lower
    );
}


} // namespace prefix_internal
