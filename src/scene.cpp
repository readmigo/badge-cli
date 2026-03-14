#include "scene.h"
#include "renderer.h"

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/IndirectLight.h>
#include <filament/Skybox.h>
#include <filament/LightManager.h>
#include <filament/TransformManager.h>
#include <filament/MaterialInstance.h>
#include <filament/Material.h>
#include <filament/Texture.h>
#include <filament/RenderableManager.h>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/MaterialProvider.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/TextureProvider.h>

#include <utils/EntityManager.h>

#include <math/mat4.h>
#include <math/vec3.h>
#include <math/quat.h>

#include <gltfio/materials/uberarchive.h>

#include <fstream>
#include <vector>
#include <cmath>

namespace badge {

SceneManager::SceneManager() = default;

SceneManager::~SceneManager() {
    destroy();
}

bool SceneManager::init(Renderer* renderer) {
    if (!renderer || !renderer->filamentEngine()) return false;
    renderer_ = renderer;

    auto* engine = renderer_->filamentEngine();

    // Create material provider (ubershader-based for gltf)
    material_provider_ = filament::gltfio::createUbershaderProvider(
        engine, UBERARCHIVE_DEFAULT_DATA, UBERARCHIVE_DEFAULT_SIZE);

    // Create asset loader
    filament::gltfio::AssetConfiguration asset_config = {};
    asset_config.engine = engine;
    asset_config.materials = material_provider_;
    asset_loader_ = filament::gltfio::AssetLoader::create(asset_config);

    // Create resource loader (for loading textures/buffers from disk)
    filament::gltfio::ResourceConfiguration res_config = {};
    res_config.engine = engine;
    res_config.normalizeSkinningWeights = true;
    resource_loader_ = new filament::gltfio::ResourceLoader(res_config);

    // Allocate light entity storage
    light_entities_ = new utils::Entity[MAX_LIGHTS];
    light_count_ = 0;

    initialized_ = true;
    return true;
}

void SceneManager::destroy() {
    if (!initialized_) return;

    unloadModel();
    clearLights();

    auto* engine = renderer_ ? renderer_->filamentEngine() : nullptr;

    if (indirect_light_ && engine) {
        engine->destroy(indirect_light_);
        indirect_light_ = nullptr;
    }
    if (skybox_ && engine) {
        engine->destroy(skybox_);
        skybox_ = nullptr;
    }

    if (resource_loader_) {
        delete resource_loader_;
        resource_loader_ = nullptr;
    }
    if (asset_loader_) {
        filament::gltfio::AssetLoader::destroy(&asset_loader_);
    }
    if (material_provider_) {
        material_provider_->destroyMaterials();
        delete material_provider_;
        material_provider_ = nullptr;
    }

    delete[] light_entities_;
    light_entities_ = nullptr;
    light_count_ = 0;

    initialized_ = false;
}

bool SceneManager::loadModel(const std::string& glb_path, BadgeLOD lod) {
    if (!initialized_ || !asset_loader_) return false;

    // Unload existing model
    unloadModel();

    // Read GLB file
    std::ifstream file(glb_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return false;

    // Create asset from GLB data
    asset_ = asset_loader_->createAsset(buffer.data(), static_cast<uint32_t>(buffer.size()));
    if (!asset_) return false;

    // Load external resources (textures, buffers)
    resource_loader_->loadResources(asset_);

    // Add all entities to the scene
    auto* scene = renderer_->filamentScene();
    if (scene) {
        scene->addEntities(asset_->getEntities(), asset_->getEntityCount());

        // Also add the root entity
        auto root = asset_->getRoot();
        scene->addEntity(root);
    }

    return true;
}

void SceneManager::unloadModel() {
    if (!asset_ || !asset_loader_) return;

    auto* scene = renderer_ ? renderer_->filamentScene() : nullptr;
    if (scene) {
        scene->removeEntities(asset_->getEntities(), asset_->getEntityCount());
    }

    if (resource_loader_) {
        resource_loader_->evictResourceData();
    }

    asset_loader_->destroyAsset(asset_);
    asset_ = nullptr;
}

void SceneManager::setTransform(const std::array<float, 3>& scale,
                                float rotation_x, float rotation_y, float rotation_z) {
    if (!asset_ || !renderer_) return;

    auto* engine = renderer_->filamentEngine();
    auto& tm = engine->getTransformManager();
    auto root = asset_->getRoot();
    auto ti = tm.getInstance(root);

    // Build transform: Scale * RotZ * RotY * RotX
    using namespace filament::math;

    mat4f s = mat4f::scaling(float3{scale[0], scale[1], scale[2]});

    float cx = std::cos(rotation_x), sx = std::sin(rotation_x);
    float cy = std::cos(rotation_y), sy = std::sin(rotation_y);
    float cz = std::cos(rotation_z), sz = std::sin(rotation_z);

    // Rotation around X — column-major: columns are basis vectors
    mat4f rx(
        float4{1,   0,   0,  0},
        float4{0,  cx,  sx,  0},
        float4{0, -sx,  cx,  0},
        float4{0,   0,   0,  1}
    );

    // Rotation around Y
    mat4f ry(
        float4{ cy, 0, -sy, 0},
        float4{  0, 1,   0, 0},
        float4{ sy, 0,  cy, 0},
        float4{  0, 0,   0, 1}
    );

    // Rotation around Z
    mat4f rz(
        float4{ cz, sz, 0, 0},
        float4{-sz, cz, 0, 0},
        float4{  0,  0, 1, 0},
        float4{  0,  0, 0, 1}
    );

    tm.setTransform(ti, s * rz * ry * rx);
}

void SceneManager::applyMaterial(const MaterialParams& params) {
    if (!asset_ || !renderer_) return;

    // Iterate over all renderable entities and update material parameters
    auto* engine = renderer_->filamentEngine();
    size_t count = asset_->getEntityCount();
    const utils::Entity* entities = asset_->getEntities();

    auto& rm = engine->getRenderableManager();

    for (size_t i = 0; i < count; i++) {
        auto ri = rm.getInstance(entities[i]);
        if (!ri.isValid()) continue;

        size_t primCount = rm.getPrimitiveCount(ri);
        for (size_t p = 0; p < primCount; p++) {
            auto* mi = rm.getMaterialInstanceAt(ri, p);
            if (!mi) continue;

            mi->setParameter("baseColor", filament::math::float3{
                params.base_color[0], params.base_color[1], params.base_color[2]
            });
            mi->setParameter("metallic", params.metalness);
            mi->setParameter("roughness", params.roughness);
            mi->setParameter("reflectance", params.reflectance);

            if (params.emissive[0] > 0 || params.emissive[1] > 0 || params.emissive[2] > 0) {
                mi->setParameter("emissive", filament::math::float4{
                    params.emissive[0], params.emissive[1], params.emissive[2], 1.0f
                });
            }
        }
    }
}

bool SceneManager::setupIBL(const std::string& ibl_path, float intensity) {
    if (!renderer_) return false;
    // IBL from KTX file would be loaded here
    // For now, use the default IBL setup
    return setupDefaultIBL(intensity);
}

bool SceneManager::setupDefaultIBL(float intensity) {
    if (!renderer_) return false;

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();

    // Clean up existing
    if (indirect_light_) {
        engine->destroy(indirect_light_);
        indirect_light_ = nullptr;
    }
    if (skybox_) {
        engine->destroy(skybox_);
        skybox_ = nullptr;
    }

    // Create a simple procedural IBL (white environment)
    // In production, this would load a KTX cubemap
    indirect_light_ = filament::IndirectLight::Builder()
        .intensity(intensity)
        .build(*engine);

    if (indirect_light_ && scene) {
        scene->setIndirectLight(indirect_light_);
    }

    // Transparent skybox for badge rendering (badges float on UI)
    skybox_ = filament::Skybox::Builder()
        .color({0.0f, 0.0f, 0.0f, 0.0f})
        .build(*engine);

    if (skybox_ && scene) {
        scene->setSkybox(skybox_);
    }

    return true;
}

void SceneManager::addDirectionalLight(const std::array<float, 3>& direction,
                                       const std::array<float, 3>& color,
                                       float intensity) {
    if (!renderer_ || light_count_ >= MAX_LIGHTS) return;

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();
    auto& em = utils::EntityManager::get();

    light_entities_[light_count_] = em.create();

    filament::LightManager::Builder(filament::LightManager::Type::DIRECTIONAL)
        .color({color[0], color[1], color[2]})
        .intensity(intensity)
        .direction({direction[0], direction[1], direction[2]})
        .castShadows(true)
        .build(*engine, light_entities_[light_count_]);

    if (scene) {
        scene->addEntity(light_entities_[light_count_]);
    }

    light_count_++;
}

void SceneManager::updateDirectionalLight(int index,
                                          const std::array<float, 3>& direction) {
    if (!renderer_ || index < 0 || index >= light_count_) return;

    auto* engine = renderer_->filamentEngine();
    auto& lm = engine->getLightManager();
    auto li = lm.getInstance(light_entities_[index]);

    if (li.isValid()) {
        lm.setDirection(li, {direction[0], direction[1], direction[2]});
    }
}

void SceneManager::clearLights() {
    if (!renderer_ || !light_entities_) return;

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();
    auto& em = utils::EntityManager::get();

    for (int i = 0; i < light_count_; i++) {
        if (scene) scene->remove(light_entities_[i]);
        engine->destroy(light_entities_[i]);
        em.destroy(light_entities_[i]);
    }
    light_count_ = 0;
}

} // namespace badge
