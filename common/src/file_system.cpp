//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "file_system.h"

#include <fmt/format.h>
#include <memory>
#include <fstream>

//----------------------------------------------------------------------------------------------------------------------

FileSystem *FileSystem::GetInstance() {
    static std::unique_ptr<FileSystem> file_system(new FileSystem());
    return file_system.get();
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<BYTE> FileSystem::ReadFile(const std::filesystem::path &path) const {
    std::basic_ifstream<BYTE> fin;

    // Open a file.
    if (path.is_absolute()) {
        fin.open(path, std::ios::in | std::ios::binary);
    } else {
        for (auto &directory : _directories) {
            fin.open(fmt::format("{}/{}", directory.string(), path.string()), std::ios::in | std::ios::binary);
            if (fin.is_open()) {
                break;
            }
        }
    }

    // Check file is opened.
    if (!fin.is_open()) {
        throw std::runtime_error(fmt::format("File isn't exist: {}.", path.string()));
    }

    // Read the contents from a file.
    return std::vector<BYTE>(std::istreambuf_iterator<BYTE>(fin), std::istreambuf_iterator<BYTE>());
}

//----------------------------------------------------------------------------------------------------------------------

void FileSystem::AddDirectory(const std::filesystem::path &directory) {
    _directories.insert(directory);
}

//----------------------------------------------------------------------------------------------------------------------
