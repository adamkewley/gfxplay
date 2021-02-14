#include "runtime_config.hpp"

#include "gfxplay_config.hpp"

struct Gfxplay_config final {
    std::filesystem::path resource_dir;
};

static Gfxplay_config load_config() {
    // TODO: a *good* application config loader would to this by loading an INI
    //       file at runtime etc. This is just hacked in to be somewhat API
    //       compatible with what's actually desired.

    Gfxplay_config cfg;
    cfg.resource_dir = GFXPLAY_RESOURCES_DIR;
    return cfg;
}

static Gfxplay_config const& get_config() {
    static Gfxplay_config cached_config = load_config();
    return cached_config;
}

std::filesystem::path gfxplay::resource_path(std::filesystem::path const& p) {
    return get_config().resource_dir / p;
}
