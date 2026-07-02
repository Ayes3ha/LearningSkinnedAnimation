#pragma once

struct ImDrawData;

bool ImGui_SoftwareRenderer_Init();
void ImGui_SoftwareRenderer_Shutdown();
void ImGui_SoftwareRenderer_RenderDrawData(ImDrawData* draw_data, unsigned int* framebuffer, int framebuffer_width, int framebuffer_height);
