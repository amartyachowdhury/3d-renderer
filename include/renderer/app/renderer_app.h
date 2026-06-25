#pragma once

#include "renderer/app/app_state.h"
#include "renderer/app/renderer_controller.h"
#include "renderer/core/asset_locator.h"
#include "renderer/platform/sdl_window.h"

#include <memory>
#include <string>

namespace renderer {

class RendererApp {
public:
    explicit RendererApp(AssetLocator assets);

    int run();

private:
    bool switch_mode(RendererMode mode, SdlWindow& window);
    void handle_global_input(SdlWindow& window, AppState& state);
    bool save_current_frame(const Framebuffer& framebuffer, RendererMode mode, std::string& error);
    void update_window_title(SdlWindow& window, const AppState& state);

    AssetLocator assets_;
    std::unique_ptr<RendererController> controller_;
    RendererMode current_mode_ = RendererMode::Rasterizer;
    std::unique_ptr<Framebuffer> framebuffer_;
};

}  // namespace renderer
