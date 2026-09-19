#include "runtime_internal.hpp"
#include "context.hpp"

#include <sstream>
#include <string>


namespace runtime_internal
{


/*
    ================================================================
    WINE CONFIGURATION
    ================================================================
*/

bool appendWineConfiguration(
    std::ostringstream& command,
    const Context& ctx
)
{
    const auto& wine =
        ctx.wine;


    /*
        ============================================================
        SYNCHRONIZATION
        ============================================================
    */

    command
        << "export WINEESYNC="
        << shellQuote(
            wine.sync.esync ? "1" : "0"
        )
        << " && ";

    command
        << "export WINEFSYNC="
        << shellQuote(
            wine.sync.fsync ? "1" : "0"
        )
        << " && ";

    command
        << "export WINE_NTSYNC="
        << shellQuote(
            wine.sync.ntsync ? "1" : "0"
        )
        << " && ";


    /*
        ============================================================
        VIRTUAL DESKTOP
        ============================================================
    */

    if(wine.display.virtualDesktop.enabled)
    {
        const int width =
            wine.display.virtualDesktop.width > 0
                ? wine.display.virtualDesktop.width
                : 640;

        const int height =
            wine.display.virtualDesktop.height > 0
                ? wine.display.virtualDesktop.height
                : 480;

        const std::string desktopSize =
            std::to_string(width) +
            "x" +
            std::to_string(height);

        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Explorer",
            "Desktop",
            "REG_SZ",
            "Default"
        );

        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Explorer\\Desktops",
            "Default",
            "REG_SZ",
            desktopSize
        );
    }
    else
    {
        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Explorer",
            "Desktop"
        );

        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Explorer\\Desktops",
            "Default"
        );
    }


    /*
        ============================================================
        X11
        ============================================================
    */

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\X11 Driver",
        "Managed",
        "REG_SZ",
        wine.display.window.managed ? "Y" : "N"
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\X11 Driver",
        "Decorated",
        "REG_SZ",
        wine.display.window.decorations ? "Y" : "N"
    );

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\X11 Driver",
        "GrabFullscreen",
        "REG_SZ",
        wine.display.window.mouseCapture ? "Y" : "N"
    );


    /*
        ============================================================
        DPI
        ============================================================
    */

    if(wine.display.dpi > 0)
    {
        appendWineRegAdd(
            command,
            "HKCU\\Control Panel\\Desktop",
            "LogPixels",
            "REG_DWORD",
            std::to_string(
                wine.display.dpi
            )
        );
    }


    /*
        ============================================================
        DIRECT3D
        ============================================================
    */

    if(
        !wine.graphics.renderer.empty() &&
        wine.graphics.renderer != "auto"
    )
    {
        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "renderer",
            "REG_SZ",
            wine.graphics.renderer
        );
    }
    else
    {
        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "renderer"
        );
    }

    if(wine.graphics.videoMemory > 0)
    {
        appendWineRegAdd(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "VideoMemorySize",
            "REG_SZ",
            std::to_string(
                wine.graphics.videoMemory
            )
        );
    }
    else
    {
        appendWineRegDelete(
            command,
            "HKCU\\Software\\Wine\\Direct3D",
            "VideoMemorySize"
        );
    }

    appendWineRegAdd(
        command,
        "HKCU\\Software\\Wine\\Direct3D",
        "strict_draw_ordering",
        "REG_SZ",
        wine.graphics.strictDrawOrdering
            ? "enabled"
            : "disabled"
    );


    /*
        ============================================================
        WINDOWS VERSION
        ============================================================

        Do NOT run winecfg -v here.

        The shared prefix is owned by both Wine and Proton.
        Reconfiguring the Windows version on every launch can
        destabilize the shared prefix.
    */


    /*
        ============================================================
        DLL OVERRIDES
        ============================================================
    */

    const std::string dllOverrides =
        buildDllOverrides(ctx);

    if(!dllOverrides.empty())
    {
        command
            << "export WINEDLLOVERRIDES="
            << shellQuote(
                dllOverrides
            )
            << " && ";
    }
    else
    {
        command
            << "unset WINEDLLOVERRIDES 2>/dev/null"
            << " && ";
    }


    /*
        ============================================================
        USER DIRECTORIES
        ============================================================
    */

    if(!appendWineUserDirectories(
        command,
        ctx
    ))
    {
        return false;
    }

    appendWineShellFolderRegistry(
        command
    );

    return true;
}


} // namespace runtime_internal
