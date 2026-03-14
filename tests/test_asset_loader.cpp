#include <gtest/gtest.h>
#include "asset_loader.h"
#include <filesystem>

TEST(AssetLoader, UnpackBadge) {
    badge::AssetLoader loader;
    auto result = loader.unpack("../tests/fixtures/test_badge.badge", "/tmp/badge_test");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->badge_id, "encounter");
    EXPECT_TRUE(std::filesystem::exists("/tmp/badge_test/models/preview.glb"));
    EXPECT_TRUE(std::filesystem::exists("/tmp/badge_test/manifest.json"));
}

TEST(AssetLoader, UnpackInvalidZipReturnsNone) {
    badge::AssetLoader loader;
    auto result = loader.unpack("/nonexistent.badge", "/tmp/badge_test2");
    EXPECT_FALSE(result.has_value());
}

TEST(AssetLoader, GetModelPathByLOD) {
    badge::AssetLoader loader;
    loader.unpack("../tests/fixtures/test_badge.badge", "/tmp/badge_test3");

    EXPECT_EQ(loader.modelPath(BADGE_LOD_THUMBNAIL), "/tmp/badge_test3/models/thumbnail.glb");
    EXPECT_EQ(loader.modelPath(BADGE_LOD_PREVIEW),   "/tmp/badge_test3/models/preview.glb");
    EXPECT_EQ(loader.modelPath(BADGE_LOD_DETAIL),    "/tmp/badge_test3/models/detail.glb");
}
