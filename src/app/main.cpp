#define SDL_MAIN_HANDLED

#include "renderer/app/renderer_app.h"
#include "renderer/core/asset_locator.h"

#include <iostream>

int main() {
    renderer::AssetLocator assets;
    renderer::RendererApp app(std::move(assets));
    return app.run();
}
