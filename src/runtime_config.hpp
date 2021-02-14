#pragma once

#include <filesystem>

namespace gfxplay {
    std::filesystem::path resource_path(std::filesystem::path const& subpath);

    template<typename... Args>
    std::filesystem::path resource_path(Args... args) {
        std::filesystem::path p;
        (p /= ... /= args);
        return resource_path(p);
    }
}
