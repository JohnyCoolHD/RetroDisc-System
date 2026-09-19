#pragma once

#include <filesystem>
#include <sstream>
#include <string>

#include "context.hpp"


namespace runtime_internal
{


/*
    ================================================================
    CONSTANTS
    ================================================================
*/

constexpr const char* CANONICAL_WINDOWS_USER = "RetroDisc";


/*
    ================================================================
    SHELL QUOTING
    ================================================================
*/

std::string shellQuote(
    const std::string& value
);


/*
    ================================================================
    COMMAND EXECUTION
    ================================================================
*/

int runCommand(
    const std::string& command
);


/*
    ================================================================
    HOME
    ================================================================
*/

std::filesystem::path getHome();


/*
    ================================================================
    STEAM ROOT
    ================================================================
*/

std::filesystem::path findSteamRoot();


/*
    ================================================================
    PROTON
    ================================================================
*/

std::filesystem::path findProton(
    const Context& ctx
);


/*
    ================================================================
    ENVIRONMENT VALIDATION
    ================================================================
*/

bool validEnvironmentName(
    const std::string& name
);


/*
    ================================================================
    PRINT ENVIRONMENT
    ================================================================
*/

void printEnvironment(
    const Context& ctx
);


/*
    ================================================================
    DLL OVERRIDES
    ================================================================
*/

std::string buildDllOverrides(
    const Context& ctx
);


/*
    ================================================================
    USER ENVIRONMENT
    ================================================================
*/

bool appendUserEnvironment(
    std::ostringstream& command,
    const Context& ctx
);


/*
    ================================================================
    RUNTIME USER
    ================================================================
*/

std::string determineRuntimeUser(
    const Context&
);


/*
    ================================================================
    FILESYSTEM ENTRY HELPERS
    ================================================================
*/

bool removeBrokenOrInvalidEntry(
    const std::filesystem::path& path
);


/*
    ================================================================
    ENSURE DIRECTORY
    ================================================================
*/

bool ensureDirectory(
    const std::filesystem::path& directory
);


/*
    ================================================================
    PREPARE CANONICAL USER DIRECTORY
    ================================================================
*/

bool ensureLegacyCompatRedirect(
    const std::filesystem::path& canonicalUser,
    const std::filesystem::path& relativeLegacyPath,
    const std::filesystem::path& linkTarget
);


bool prepareCanonicalUserDirectory(
    const std::filesystem::path& usersDirectory
);


bool migrateAndSymlinkLegacyUser(
    const std::filesystem::path& usersDirectory,
    const std::filesystem::path& persistentUsersDirectory,
    const std::filesystem::path& sourceUsersDirectory,
    const std::string& legacyName
);


/*
    ================================================================
    PREPARE RUNTIME USER
    ================================================================
*/

bool prepareRuntimeUser(
    const Context& ctx,
    const std::string& runtimeUser
);


/*
    ================================================================
    CLEANUP RUNTIME USER
    ================================================================
*/

void cleanupRuntimeUser(
    const Context&,
    const std::string&
);


/*
    ================================================================
    WINE REGISTRY
    ================================================================
*/

void appendWineRegAdd(
    std::ostringstream& command,
    const std::string& key,
    const std::string& valueName,
    const std::string& type,
    const std::string& value
);


void appendWineRegDelete(
    std::ostringstream& command,
    const std::string& key,
    const std::string& valueName
);


/*
    ================================================================
    WINE USER DIRECTORIES
    ================================================================
*/

bool appendWineUserDirectories(
    std::ostringstream& command,
    const Context& ctx
);


/*
    ================================================================
    WINE SHELL FOLDERS
    ================================================================
*/

void appendWineShellFolderRegistry(
    std::ostringstream& command
);


/*
    ================================================================
    WINE CONFIGURATION
    ================================================================
*/

bool appendWineConfiguration(
    std::ostringstream& command,
    const Context& ctx
);


/*
    ================================================================
    WINE GAME LAUNCH
    ================================================================
*/

void appendWineGameLaunch(
    std::ostringstream& command,
    const Context& ctx
);


/*
    ================================================================
    BUILD WINE COMMAND
    ================================================================
*/

std::string buildWineCommand(
    const Context& ctx,
    const std::string&
);


std::string buildProtonCommand(
    const Context& ctx,
    const std::string&
);


} // namespace runtime_internal
