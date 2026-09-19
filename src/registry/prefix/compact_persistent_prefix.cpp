#include "prefix_internal.hpp"

#include <filesystem>
#include <functional>
#include <iostream>
#include <system_error>
#include <vector>


namespace prefix_internal
{


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


} // namespace prefix_internal
