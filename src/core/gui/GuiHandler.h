#ifndef AETHERION_KINETICA_GUI_HANDLER_H
#define AETHERION_KINETICA_GUI_HANDLER_H

#include <functional>

union SDL_Event;
class SDL_Window;
class SDL_Renderer;

using GuiEvent = SDL_Event;
using GuiRenderer = SDL_Renderer;
using GuiWindow = SDL_Window;

namespace engine::core::gui {
    class GuiHandler {
    public:
        ~GuiHandler();

        GuiHandler() = default;

        GuiHandler(GuiHandler const &) = delete;

        void operator=(GuiHandler const &) = delete;

        void init(GuiWindow *window);

        void renderGui(GuiWindow *window) const;

        void handleEvent(const GuiEvent &event) const;


    private:
        GuiRenderer *renderer{nullptr};
        std::function<void()> renderFunction{[]{}};
    };
}

#endif //AETHERION_KINETICA_GUI_HANDLER_H
