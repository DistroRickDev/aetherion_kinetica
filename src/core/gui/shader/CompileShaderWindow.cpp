#include "CompileShaderWindow.h"

#include <algorithm>
#include <filesystem>
#include <vector>

#include <core/gui/builder/GuiBuilder.h>
#include <core/logger/SdlLoggerHandler.h>
#include <core/shaders/ShaderHandler.h>

namespace {
    using namespace engine::core;
    using Logger = logger::SdlLoggerHandler;

    std::vector<std::string> filePathVecFromFs(const std::filesystem::directory_iterator &fsIter,
                                               const bool sorted = true) {
        std::vector<std::string> res{};
        for (const auto &entry: fsIter) {
            res.push_back(entry.path().string());
        }
        if (sorted) {
            std::ranges::sort(res);
        }
        return res;
    }

    struct CompileShaderContext {
        bool showCompileShaderWindowState{false};
        bool showPersistCompileShaderWindowState{false};
        std::filesystem::path currentDirectory{std::filesystem::path(std::getenv("HOME"))};
        std::string selectedPath{};
        std::string storeFileName{};
        shaders::GlslToSpirvHolder shaderHolder{};
    };

    const auto kState{std::make_unique<CompileShaderContext>()};

    void goUpDirectory() {
        kState->currentDirectory = kState->currentDirectory.parent_path();
    }

    void goHomeDirectory() {
        kState->currentDirectory = std::filesystem::path(std::getenv("HOME"));
    }

    bool isSelectedPathSelected(const std::string &currentPath) {
        return currentPath == kState->selectedPath;
    }

    void selectDirectory(const std::string &currentPathStr) {
        if (const auto currentPath{std::filesystem::path(currentPathStr)}; std::filesystem::is_directory(currentPath)) {
            kState->currentDirectory = currentPath;
        }
    }

    void selectPath(const std::string &currentPathStr) {
        if (const auto currentPath{std::filesystem::path(currentPathStr)}; std::filesystem::is_directory(currentPath)) {
            kState->currentDirectory = currentPath;
            return;
        }
        kState->selectedPath = currentPathStr;
    }

    void onCompileShaderClicked() {
        kState->shaderHolder = shaders::ShaderHandler::compileShaderFromFile(kState->selectedPath);
        kState->selectedPath = {};
        gui::setShowCompileShaderWindow(false);
    }

    void onStoreShaderClicked() {
        const auto fullFilePath{std::format("{}/{}", kState->currentDirectory.string(), kState->storeFileName)};
        shaders::ShaderHandler::storeCompiledShader(kState->shaderHolder.spirvBuffer, fullFilePath);
        kState->showPersistCompileShaderWindowState = false;
    }
}

bool gui::showCompileShaderWindow() {
    return kState->showCompileShaderWindowState;
}

void gui::setShowCompileShaderWindow(const bool show) {
    kState->showCompileShaderWindowState = show;
}

void gui::compileShaderWindow() {
    // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    if (ImGui::Begin("Select GLSL Shader to compile", &kState->showCompileShaderWindowState,
                     ImGuiWindowFlags_NoCollapse)) {
        addButton("Up", goUpDirectory);
        ImGui::SameLine();
        addButton("Home", goHomeDirectory);

        addListBox("##ShaderCompilerFileSelectListBox",
                   filePathVecFromFs(std::filesystem::directory_iterator{kState->currentDirectory}),
                   isSelectedPathSelected,
                   selectPath);

        addButton("Compile Shader", onCompileShaderClicked);
        ImGui::SameLine();
        ImGui::Checkbox("Persist Compiled Shader", &kState->showPersistCompileShaderWindowState);
        ImGui::End();
    }
}

bool gui::showPersistShaderWindow() {
    return kState->showPersistCompileShaderWindowState && !kState->showCompileShaderWindowState;
}

void gui::persistShaderWindow() {
    if (ImGui::Begin("Persist SPIRV shader", &kState->showPersistCompileShaderWindowState,
                     ImGuiWindowFlags_NoCollapse)) {
        addButton("Up", goUpDirectory);
        ImGui::SameLine();
        addButton("Home", goHomeDirectory);
        addInputText("FileName:", &kState->storeFileName);
        addListBox("##ShaderCompilerFileSelectListBox",
                   filePathVecFromFs(std::filesystem::directory_iterator{kState->currentDirectory}),
                   isSelectedPathSelected,
                   selectDirectory);
        addButton("Store Shader", onStoreShaderClicked);
        ImGui::End();
    }
}
