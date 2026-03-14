#pragma once

#include "badge_engine/types.h"
#include "config.h"
#include <string>
#include <optional>

namespace badge {

struct UnpackResult {
    std::string badge_id;
    std::string rarity;
    std::string extract_dir;
    BadgeManifest manifest;
};

class AssetLoader {
public:
    std::optional<UnpackResult> unpack(const std::string& badge_path, const std::string& dest_dir);

    std::string modelPath(BadgeLOD lod) const;
    std::string ceremonyPath() const;
    std::string manifestPath() const;

    bool isLoaded() const { return loaded_; }

private:
    bool loaded_ = false;
    std::string extract_dir_;
    BadgeManifest manifest_;
};

} // namespace badge
