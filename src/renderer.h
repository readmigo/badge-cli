#pragma once

#include "badge_engine/types.h"
#include <cstdint>

namespace filament {
class Engine;
class Renderer;
class SwapChain;
class View;
class Scene;
class Camera;
class ColorGrading;
} // namespace filament

namespace utils {
class Entity;
} // namespace utils

namespace badge {

struct RendererConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    BadgeRenderMode mode = BADGE_RENDER_EMBEDDED;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(void* native_surface, uint32_t width, uint32_t height);
    void destroy();

    void setRenderMode(BadgeRenderMode mode);
    BadgeRenderMode renderMode() const { return mode_; }

    void resize(uint32_t width, uint32_t height);

    bool beginFrame();
    void render();
    void endFrame();

    int readPixels(uint8_t* buffer, uint32_t width, uint32_t height);

    filament::Engine* filamentEngine() const { return engine_; }
    filament::Scene* filamentScene() const { return scene_; }
    filament::View* filamentView() const { return view_; }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

private:
    void setupPostProcessing();
    void setupCamera();

    filament::Engine* engine_ = nullptr;
    filament::Renderer* renderer_ = nullptr;
    filament::SwapChain* swap_chain_ = nullptr;
    filament::View* view_ = nullptr;
    filament::Scene* scene_ = nullptr;
    filament::Camera* camera_ = nullptr;
    filament::ColorGrading* color_grading_ = nullptr;
    utils::Entity* camera_entity_ = nullptr;

    BadgeRenderMode mode_ = BADGE_RENDER_EMBEDDED;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool initialized_ = false;
};

} // namespace badge
