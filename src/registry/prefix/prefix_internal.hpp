#pragma once

#include <filesystem>
#include <string>

#include "context.hpp"


namespace prefix_internal
{


std::filesystem::path getHome();


std::string getUnixUsername();


std::filesystem::path findSteamRootForProton(
    const std::filesystem::path& proton
);


/*
    ================================================================
    SANITIZE IDENTIFIER
    ================================================================
*/

std::string sanitizeIdentifier(
    const std::string& raw
);


/*
    ================================================================
    RESOLVE PROTON VERSION IDENTIFIER
    ================================================================
*/

std::string resolveProtonVersionIdentifier(
    const std::filesystem::path& protonBinary
);


/*
    ================================================================
    RESOLVE WINE VERSION IDENTIFIER
    ================================================================
*/

std::string resolveWineVersionIdentifier();


/*
    ================================================================
    RESOLVE GLOBAL PREFIX DIRECTORY
    ================================================================
*/

std::filesystem::path resolveGlobalPrefixDirectory(
    const std::filesystem::path& home,
    const std::string& runtimeFolder,
    const std::string& versionIdentifier
);


bool filesEqual(
    const std::filesystem::path& left,
    const std::filesystem::path& right
);


bool isPersonalHomeSymlink(
    const std::filesystem::path& source,
    const std::filesystem::path& linkTarget
);


bool ensureWineUserSymlink(
    const std::filesystem::path& persistentPrefix
);


bool compactPersistentPrefix(
    const std::filesystem::path& upper,
    const std::filesystem::path& base
);

bool commitPersistentPrefixOverlay(
    const std::filesystem::path& runtimeUpper,
    const std::filesystem::path& persistentUpper,
    const std::filesystem::path& lower
);


bool copyBundledPrefix(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    const std::filesystem::path& base
);


/*
    ================================================================
    INITIALIZE GLOBAL PREFIX
    ================================================================
*/

bool initializeGlobalPrefix(
    Context& ctx
);


/*
    ================================================================
    PREPARE GAME PREFIX
    ================================================================
*/

bool prepareGamePrefix(Context& ctx);


} // namespace prefix_internal
