#pragma once

#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>


namespace prefix_internal
{


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


} // namespace prefix_internal
