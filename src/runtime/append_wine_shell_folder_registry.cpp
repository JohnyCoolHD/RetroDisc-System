#include "runtime_internal.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    WINE SHELL FOLDERS
    ================================================================
*/

void appendWineShellFolderRegistry(
    std::ostringstream& command
)
{
    const std::string user =
        "C:\\users\\RetroDisc";

    const std::string documents =
        user + "\\Documents";

    const std::string roaming =
        user + "\\AppData\\Roaming";

    const std::string local =
        user + "\\AppData\\Local";

    const std::string desktop =
        user + "\\Desktop";

    const std::string downloads =
        user + "\\Downloads";

    const std::string pictures =
        user + "\\Pictures";

    const std::string music =
        user + "\\Music";

    const std::string videos =
        user + "\\Videos";


    /*
        ============================================================
        SHELL FOLDERS
        ============================================================
    */

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "Personal",
        "REG_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Documents",
        "REG_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "AppData",
        "REG_SZ",
        roaming
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "Local AppData",
        "REG_SZ",
        local
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "Desktop",
        "REG_SZ",
        desktop
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "{374DE290-123F-4565-9164-39C4925E467B}",
        "REG_SZ",
        downloads
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Pictures",
        "REG_SZ",
        pictures
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Music",
        "REG_SZ",
        music
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        "My Video",
        "REG_SZ",
        videos
    );


    /*
        ============================================================
        USER SHELL FOLDERS
        ============================================================
    */

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "Personal",
        "REG_EXPAND_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Documents",
        "REG_EXPAND_SZ",
        documents
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "AppData",
        "REG_EXPAND_SZ",
        roaming
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "Local AppData",
        "REG_EXPAND_SZ",
        local
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "Desktop",
        "REG_EXPAND_SZ",
        desktop
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "{374DE290-123F-4565-9164-39C4925E467B}",
        "REG_EXPAND_SZ",
        downloads
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Pictures",
        "REG_EXPAND_SZ",
        pictures
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Music",
        "REG_EXPAND_SZ",
        music
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
        "My Video",
        "REG_EXPAND_SZ",
        videos
    );
}


} // namespace runtime_internal
