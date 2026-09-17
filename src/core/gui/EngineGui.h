#ifndef AETHERION_KINETICA_GUI_HANDLER_H
#define AETHERION_KINETICA_GUI_HANDLER_H

#include <atomic>
#include <thread>

union SDL_Event;
class SDL_Window;
class SDL_Renderer;

using GuiEvent = SDL_Event;
using GuiRenderer = SDL_Renderer;
using GuiWindow = SDL_Window;

namespace engine::core::gui {
    class EngineGui {
    public:
        ~EngineGui();

        EngineGui() = default;

        EngineGui(EngineGui const &) = delete;

        void operator=(EngineGui const &) = delete;

        void init(GuiWindow *window);

        void renderGui(GuiWindow *window);

        void handleEvent(const GuiEvent &event) const;

    private:
        GuiRenderer *renderer{nullptr};
    };
}

#endif //AETHERION_KINETICA_GUI_HANDLER_H
