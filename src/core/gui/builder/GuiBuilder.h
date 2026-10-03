#ifndef AETHERION_KINETICA_GUI_BUILDER_H
#define AETHERION_KINETICA_GUI_BUILDER_H
#include <format>
#include <functional>
#include <string>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace engine::core::gui {
    inline void addButton(const std::string &label, const std::function<void()> &onButtonClicked = [] {
                          }) {
        if (ImGui::Button(label.c_str())) {
            onButtonClicked();
        }
    }

    struct MenuItem {
        const std::string label;
        const std::string shortcut;
        const std::function<void()> onItemClicked{
            [] {
                /* do nothing */
            }
        };
    };

    template<std::size_t MenuItemCount>
    void addMenu(const std::string &menuLabel, std::array<MenuItem, MenuItemCount> &&items) {
        if (ImGui::BeginMenu(menuLabel.c_str())) {
            for (const auto &item: items) {
                if (ImGui::MenuItem(item.label.c_str(), item.shortcut.c_str())) {
                    item.onItemClicked();
                }
            }
            ImGui::EndMenu();
        }
    }

    inline void addListBox(const std::string &label, const std::vector<std::string> &itemList,
                           const std::function<bool(const std::string&)> &itemSelectedCheck,
                           const std::function<void(const std::string&)> &selectableLogic) {
        if (ImGui::BeginListBox(label.c_str(), ImVec2(-FLT_MIN, 6 * ImGui::GetTextLineHeightWithSpacing()))) {
            for (const auto &item: itemList) {
                const auto isItemSelected = itemSelectedCheck(item);
                if (ImGui::Selectable(item.c_str(), isItemSelected)) {
                    selectableLogic(item);
                }
            }
            ImGui::EndListBox();
        }
    }

    inline void addInputText(const std::string &label, std::string *text) {
        ImGui::InputText(label.c_str(), text);
    }
}


#endif //AETHERION_KINETICA_GUI_BUILDER_H
