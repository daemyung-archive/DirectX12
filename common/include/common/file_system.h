//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include <Windows.h>
#include <filesystem>
#include <vector>
#include <set>

//----------------------------------------------------------------------------------------------------------------------

class FileSystem {
public:
    //! Retrieve a file system.
    //! \return A file system.
    [[nodiscard]]
    static FileSystem* GetInstance();

    //! Read a file.
    //! \param path A file path.
    //! \return The contents of a file.
    [[nodiscard]]
    std::vector<BYTE> ReadFile(const std::filesystem::path &path) const;

    //! Add a directory to find a file.
    //! \param directory A directory to find a file.
    void AddDirectory(const std::filesystem::path &directory);

private:
    std::set<std::filesystem::path> _directories = {COMMON_ASSET_DIR};
};

//----------------------------------------------------------------------------------------------------------------------

#endif