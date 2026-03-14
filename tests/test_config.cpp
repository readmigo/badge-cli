#include <gtest/gtest.h>
#include "config.h"

TEST(Config, ParseManifest) {
    auto manifest = badge::Config::parseManifest("../tests/fixtures/manifest_sample.json");
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->badge_id, "encounter");
    EXPECT_EQ(manifest->rarity, "RARE");
    EXPECT_EQ(manifest->tier, 1);
    EXPECT_EQ(manifest->format_version, 1);
}

TEST(Config, ParseHexColor) {
    auto rgb = badge::Config::hexToRGB("#FFD700");
    EXPECT_NEAR(rgb[0], 1.0f, 0.01f);
    EXPECT_NEAR(rgb[1], 0.843f, 0.01f);
    EXPECT_NEAR(rgb[2], 0.0f, 0.01f);
}

TEST(Config, InvalidManifestReturnsNone) {
    auto manifest = badge::Config::parseManifest("/nonexistent.json");
    EXPECT_FALSE(manifest.has_value());
}

TEST(Config, ParseHexColorWithoutHash) {
    auto rgb = badge::Config::hexToRGB("5C4400");
    EXPECT_NEAR(rgb[0], 0.361f, 0.01f);
    EXPECT_NEAR(rgb[1], 0.267f, 0.01f);
    EXPECT_NEAR(rgb[2], 0.0f, 0.01f);
}

TEST(Config, ManifestHasLocalizedNames) {
    auto manifest = badge::Config::parseManifest("../tests/fixtures/manifest_sample.json");
    ASSERT_TRUE(manifest.has_value());
    EXPECT_EQ(manifest->name.at("en"), "Encounter");
}

TEST(Config, ManifestHasMaterialOverrides) {
    auto manifest = badge::Config::parseManifest("../tests/fixtures/manifest_sample.json");
    ASSERT_TRUE(manifest.has_value());
    ASSERT_TRUE(manifest->metalness.has_value());
    EXPECT_FLOAT_EQ(*manifest->metalness, 0.95f);
    ASSERT_TRUE(manifest->roughness.has_value());
    EXPECT_FLOAT_EQ(*manifest->roughness, 0.18f);
}
