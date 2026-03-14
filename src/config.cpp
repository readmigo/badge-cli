#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>

namespace badge {

std::optional<BadgeManifest> Config::parseManifest(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return parseManifestString(content);
}

std::optional<BadgeManifest> Config::parseManifestString(const std::string& json_str) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_str);
    } catch (...) {
        return std::nullopt;
    }

    BadgeManifest manifest;
    manifest.format_version = j.value("format_version", 0);
    manifest.badge_id = j.value("badge_id", "");
    manifest.tier = j.value("tier", 0);
    manifest.rarity = j.value("rarity", "");
    manifest.template_name = j.value("template", "");

    // Parse localized names
    if (j.contains("name") && j["name"].is_object()) {
        for (auto& [key, val] : j["name"].items()) {
            manifest.name[key] = val.get<std::string>();
        }
    }

    // Parse material overrides
    if (j.contains("material")) {
        auto& mat = j["material"];
        if (mat.contains("base_color")) {
            if (mat["base_color"].is_string()) {
                manifest.base_color = hexToRGB(mat["base_color"].get<std::string>());
            }
        }
        if (mat.contains("metalness")) {
            manifest.metalness = mat["metalness"].get<float>();
        }
        if (mat.contains("roughness")) {
            manifest.roughness = mat["roughness"].get<float>();
        }
        if (mat.contains("emissive")) {
            if (mat["emissive"].is_string()) {
                manifest.emissive = hexToRGB(mat["emissive"].get<std::string>());
            }
        }
    }

    return manifest;
}

std::array<float, 3> Config::hexToRGB(const std::string& hex) {
    std::string h = hex;
    if (!h.empty() && h[0] == '#') {
        h = h.substr(1);
    }

    if (h.size() != 6) {
        return {0.0f, 0.0f, 0.0f};
    }

    unsigned long val = std::strtoul(h.c_str(), nullptr, 16);
    float r = static_cast<float>((val >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((val >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>(val & 0xFF) / 255.0f;

    return {r, g, b};
}

} // namespace badge
