#pragma once

#include <string>
#include <optional>
#include <array>
#include <unordered_map>

namespace badge {

struct BadgeManifest {
    int format_version = 0;
    std::string badge_id;
    std::unordered_map<std::string, std::string> name;  // locale -> name
    int tier = 0;
    std::string rarity;
    std::string template_name;

    // Material overrides from manifest
    std::optional<std::array<float, 3>> base_color;
    std::optional<float> metalness;
    std::optional<float> roughness;
    std::optional<std::array<float, 3>> emissive;
};

class Config {
public:
    static std::optional<BadgeManifest> parseManifest(const std::string& path);
    static std::optional<BadgeManifest> parseManifestString(const std::string& json_str);
    static std::array<float, 3> hexToRGB(const std::string& hex);
};

} // namespace badge
