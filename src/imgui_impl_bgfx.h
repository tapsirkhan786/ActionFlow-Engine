#pragma once

#include <imgui.h>
#include <bgfx/bgfx.h>

namespace ImGui::BGFX {

bool Init(int viewId);
void Shutdown();
void NewFrame();
void RenderDrawData(ImDrawData* drawData);

} // namespace