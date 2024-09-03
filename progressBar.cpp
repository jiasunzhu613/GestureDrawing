#include "vcpkg/vcpkg_installed/x64-osx/include/imgui_internal.h"

namespace ImGui {
    bool ProgressBar(const char* label, const ImVec2& size_arg = ImVec2(0, 0)) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false; 
        
        ImGuiContext& g = *GImGui; 
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);


        return true;
    }
}
