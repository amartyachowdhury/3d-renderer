#pragma once

#include "renderer/app/app_state.h"
#include "renderer/core/asset_locator.h"
#include "renderer/core/framebuffer.h"
#include "renderer/platform/sdl_window.h"

#include <memory>
#include <string>

namespace renderer {

class RendererController {
public:
    virtual ~RendererController() = default;

    virtual bool init(const AssetLocator& assets, std::string& error) = 0;
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void update(SdlWindow& window, double dt) = 0;
    virtual void render(Framebuffer& framebuffer) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual std::string mode_label() const = 0;
    virtual std::string help_text() const = 0;
};

std::unique_ptr<RendererController> make_controller(RendererMode mode);

}  // namespace renderer
