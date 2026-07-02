#include "imgui_sw.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "imgui.h"

namespace {

struct SoftwareTexture {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba;
};

struct Color32 {
    int r;
    int g;
    int b;
    int a;
};

SoftwareTexture* g_font_texture = NULL;

int ClampByte(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return value;
}

Color32 UnpackImGuiColor(ImU32 color) {
    Color32 result = {};
    result.r = (int)(color & 0xffu);
    result.g = (int)((color >> 8) & 0xffu);
    result.b = (int)((color >> 16) & 0xffu);
    result.a = (int)((color >> 24) & 0xffu);
    return result;
}

unsigned int PackBgra(int r, int g, int b, int a) {
    return (unsigned int)(
        (ClampByte(b)) |
        (ClampByte(g) << 8) |
        (ClampByte(r) << 16) |
        (ClampByte(a) << 24));
}

Color32 SampleTexture(const SoftwareTexture* texture, float u, float v) {
    if (texture == NULL || texture->rgba.empty()) {
        return { 255, 255, 255, 255 };
    }

    const float clamped_u = std::max(0.0f, std::min(1.0f, u));
    const float clamped_v = std::max(0.0f, std::min(1.0f, v));
    const int x = std::min((int)(clamped_u * (float)(texture->width - 1) + 0.5f), texture->width - 1);
    const int y = std::min((int)(clamped_v * (float)(texture->height - 1) + 0.5f), texture->height - 1);
    const int index = (y * texture->width + x) * 4;

    return {
        texture->rgba[index + 0],
        texture->rgba[index + 1],
        texture->rgba[index + 2],
        texture->rgba[index + 3]
    };
}

unsigned int AlphaBlend(unsigned int dst_color, int src_r, int src_g, int src_b, int src_a) {
    if (src_a <= 0) {
        return dst_color;
    }

    if (src_a >= 255) {
        return PackBgra(src_r, src_g, src_b, 255);
    }

    const int dst_b = (int)(dst_color & 0xffu);
    const int dst_g = (int)((dst_color >> 8) & 0xffu);
    const int dst_r = (int)((dst_color >> 16) & 0xffu);
    const int dst_a = (int)((dst_color >> 24) & 0xffu);
    const int inv_a = 255 - src_a;

    const int out_r = (src_r * src_a + dst_r * inv_a) / 255;
    const int out_g = (src_g * src_a + dst_g * inv_a) / 255;
    const int out_b = (src_b * src_a + dst_b * inv_a) / 255;
    const int out_a = src_a + (dst_a * inv_a) / 255;
    return PackBgra(out_r, out_g, out_b, out_a);
}

float EdgeFunction(const ImVec2& a, const ImVec2& b, float px, float py) {
    return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
}

void RasterizeTriangle(
    const ImDrawVert& v0,
    const ImDrawVert& v1,
    const ImDrawVert& v2,
    const ImVec4& clip_rect,
    const SoftwareTexture* texture,
    unsigned int* framebuffer,
    int framebuffer_width,
    int framebuffer_height) {
    const ImVec2 p0 = v0.pos;
    const ImVec2 p1 = v1.pos;
    const ImVec2 p2 = v2.pos;

    const float area = EdgeFunction(p0, p1, p2.x, p2.y);
    if (std::fabs(area) < 1e-5f) {
        return;
    }

    const float min_x = std::min(p0.x, std::min(p1.x, p2.x));
    const float min_y = std::min(p0.y, std::min(p1.y, p2.y));
    const float max_x = std::max(p0.x, std::max(p1.x, p2.x));
    const float max_y = std::max(p0.y, std::max(p1.y, p2.y));

    const int clip_min_x = std::max(0, (int)std::floor(clip_rect.x));
    const int clip_min_y = std::max(0, (int)std::floor(clip_rect.y));
    const int clip_max_x = std::min(framebuffer_width - 1, (int)std::ceil(clip_rect.z) - 1);
    const int clip_max_y = std::min(framebuffer_height - 1, (int)std::ceil(clip_rect.w) - 1);

    const int x0 = std::max(clip_min_x, (int)std::floor(min_x));
    const int y0 = std::max(clip_min_y, (int)std::floor(min_y));
    const int x1 = std::min(clip_max_x, (int)std::ceil(max_x));
    const int y1 = std::min(clip_max_y, (int)std::ceil(max_y));

    if (x0 > x1 || y0 > y1) {
        return;
    }

    const Color32 c0 = UnpackImGuiColor(v0.col);
    const Color32 c1 = UnpackImGuiColor(v1.col);
    const Color32 c2 = UnpackImGuiColor(v2.col);
    const float inv_area = 1.0f / area;

    for (int y = y0; y <= y1; ++y) {
        const float py = (float)y + 0.5f;
        for (int x = x0; x <= x1; ++x) {
            const float px = (float)x + 0.5f;
            const float w0 = EdgeFunction(p1, p2, px, py);
            const float w1 = EdgeFunction(p2, p0, px, py);
            const float w2 = EdgeFunction(p0, p1, px, py);

            if ((area > 0.0f && (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)) ||
                (area < 0.0f && (w0 > 0.0f || w1 > 0.0f || w2 > 0.0f))) {
                continue;
            }

            const float b0 = w0 * inv_area;
            const float b1 = w1 * inv_area;
            const float b2 = w2 * inv_area;

            const float u = v0.uv.x * b0 + v1.uv.x * b1 + v2.uv.x * b2;
            const float v = v0.uv.y * b0 + v1.uv.y * b1 + v2.uv.y * b2;

            const Color32 texel = SampleTexture(texture, u, v);

            const int src_r = ClampByte((int)((c0.r * b0 + c1.r * b1 + c2.r * b2) * texel.r / 255.0f));
            const int src_g = ClampByte((int)((c0.g * b0 + c1.g * b1 + c2.g * b2) * texel.g / 255.0f));
            const int src_b = ClampByte((int)((c0.b * b0 + c1.b * b1 + c2.b * b2) * texel.b / 255.0f));
            const int src_a = ClampByte((int)((c0.a * b0 + c1.a * b1 + c2.a * b2) * texel.a / 255.0f));

            const int framebuffer_y = framebuffer_height - 1 - y;
            const int index = framebuffer_y * framebuffer_width + x;
            framebuffer[index] = AlphaBlend(framebuffer[index], src_r, src_g, src_b, src_a);
        }
    }
}

bool CreateFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels = NULL;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (pixels == NULL || width <= 0 || height <= 0) {
        return false;
    }

    delete g_font_texture;
    g_font_texture = new SoftwareTexture();
    g_font_texture->width = width;
    g_font_texture->height = height;
    g_font_texture->rgba.assign(pixels, pixels + width * height * 4);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_font_texture);
    return true;
}

void DestroyFontsTexture() {
    if (ImGui::GetCurrentContext() != NULL) {
        ImGui::GetIO().Fonts->SetTexID((ImTextureID)0);
    }
    delete g_font_texture;
    g_font_texture = NULL;
}

} // namespace

bool ImGui_SoftwareRenderer_Init() {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_sw";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    return CreateFontsTexture();
}

void ImGui_SoftwareRenderer_Shutdown() {
    if (ImGui::GetCurrentContext() != NULL) {
        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = NULL;
        io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    }
    DestroyFontsTexture();
}

void ImGui_SoftwareRenderer_RenderDrawData(
    ImDrawData* draw_data,
    unsigned int* framebuffer,
    int framebuffer_width,
    int framebuffer_height) {
    if (draw_data == NULL || framebuffer == NULL || framebuffer_width <= 0 || framebuffer_height <= 0) {
        return;
    }

    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) {
        return;
    }

    const ImVec2 clip_off = draw_data->DisplayPos;

    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
        const ImDrawList* draw_list = draw_data->CmdLists[list_index];
        const ImDrawVert* vertex_buffer = draw_list->VtxBuffer.Data;
        const ImDrawIdx* index_buffer = draw_list->IdxBuffer.Data;

        for (int cmd_index = 0; cmd_index < draw_list->CmdBuffer.Size; ++cmd_index) {
            const ImDrawCmd* command = &draw_list->CmdBuffer[cmd_index];

            if (command->UserCallback != NULL) {
                if (command->UserCallback == ImDrawCallback_ResetRenderState) {
                    continue;
                }
                command->UserCallback(draw_list, command);
                continue;
            }

            ImVec4 clip_rect = command->ClipRect;
            clip_rect.x -= clip_off.x;
            clip_rect.y -= clip_off.y;
            clip_rect.z -= clip_off.x;
            clip_rect.w -= clip_off.y;

            if (clip_rect.x >= framebuffer_width || clip_rect.y >= framebuffer_height ||
                clip_rect.z <= 0.0f || clip_rect.w <= 0.0f) {
                continue;
            }

            const SoftwareTexture* texture = (const SoftwareTexture*)(intptr_t)command->GetTexID();
            const ImDrawIdx* command_indices = index_buffer + command->IdxOffset;

            for (unsigned int elem = 0; elem + 2 < command->ElemCount; elem += 3) {
                ImDrawVert v0 = vertex_buffer[command_indices[elem + 0] + command->VtxOffset];
                ImDrawVert v1 = vertex_buffer[command_indices[elem + 1] + command->VtxOffset];
                ImDrawVert v2 = vertex_buffer[command_indices[elem + 2] + command->VtxOffset];

                v0.pos.x -= clip_off.x;
                v0.pos.y -= clip_off.y;
                v1.pos.x -= clip_off.x;
                v1.pos.y -= clip_off.y;
                v2.pos.x -= clip_off.x;
                v2.pos.y -= clip_off.y;

                RasterizeTriangle(v0, v1, v2, clip_rect, texture, framebuffer, framebuffer_width, framebuffer_height);
            }
        }
    }
}
