#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>

#include "imgui.h"
#include "imgui_impl_win32.h"

#include "public.h"
#include "tool.h"
#include "geometry.h"
#include "mesh.h"
#include "graphic.h"
#include "tgaimage.h"
#include "imgui_sw.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

const char* kWindowClassName = "RedRendererImGuiWindow";
const char* kWindowTitle = "RedRenderer + ImGui";
const char* kObjPath = "./obj/diablo3_pose/diablo3_pose.obj";
const char* kTexPath = "./obj/diablo3_pose/diablo3_pose_diffuse.tga";

enum RenderMode {
    RenderMode_Textured = 0,
    RenderMode_Gouraud = 1,
    RenderMode_Points = 2,
};

struct BitmapSurface {
    HDC dc = NULL;
    HBITMAP bitmap = NULL;
    HBITMAP old_bitmap = NULL;
    unsigned int* pixels = NULL;
    BITMAPINFO info = {};
};

struct FrameStats {
    int frame_count = 0;
    float fps = 0.0f;
    float avg_ms = 0.0f;
    float frame_ms = 0.0f;
    double sample_start = 0.0;
};

struct SceneState {
    Vec3f light_pos;
    Vec3f camera_pos;
    Vec3f center;
    Vec3f up;
    float fov = 45.0f;
    float aspect = 4.0f / 3.0f;
    int near_plane = 10;
    int far_plane = 1000;
    int render_mode = RenderMode_Textured;
    bool show_demo_window = false;
    bool show_metrics_window = false;
    bool assets_available = false;
};

struct AppState {
    HWND hwnd = NULL;
    BitmapSurface surface;
    FrameStats stats;
    SceneState scene;
    Mesh* mesh = NULL;
    TGAImage* image = NULL;
    Renderer* renderer = NULL;
    float* zbuffer = NULL;
    int keys[512] = {};
    short mouse_wheel_delta = 0;
    bool running = true;
    COLORREF white = 0;
    COLORREF black = 0;
    COLORREF background = 0;
};

AppState g_app;

bool FileExists(const char* path) {
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

void InitConsole() {
    if (!AllocConsole()) {
        return;
    }

    FILE* stream = NULL;
    freopen_s(&stream, "CON", "r", stdin);
    freopen_s(&stream, "CON", "w", stdout);
    freopen_s(&stream, "CON", "w", stderr);
}

void ResetSceneState() {
    g_app.scene.light_pos = Vec3f(1.0f, 1.0f, 1.5f);
    g_app.scene.camera_pos = Vec3f(0.0f, 0.0f, 5.0f);
    g_app.scene.center = Vec3f(0.0f, 0.0f, 0.0f);
    g_app.scene.up = Vec3f(0.0f, -1.0f, 0.0f);
    g_app.scene.fov = 45.0f;
    g_app.scene.aspect = 4.0f / 3.0f;
    g_app.scene.near_plane = 10;
    g_app.scene.far_plane = 1000;
}

bool CreateBitmapSurface(HWND hwnd, BitmapSurface& surface) {
    ZeroMemory(&surface.info, sizeof(surface.info));
    surface.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    surface.info.bmiHeader.biWidth = width;
    surface.info.bmiHeader.biHeight = height;
    surface.info.bmiHeader.biPlanes = 1;
    surface.info.bmiHeader.biBitCount = 32;
    surface.info.bmiHeader.biCompression = BI_RGB;
    surface.info.bmiHeader.biSizeImage = width * height * 4;

    HDC window_dc = GetDC(hwnd);
    surface.dc = CreateCompatibleDC(window_dc);
    ReleaseDC(hwnd, window_dc);

    if (surface.dc == NULL) {
        return false;
    }

    void* raw_pixels = NULL;
    surface.bitmap = CreateDIBSection(surface.dc, &surface.info, DIB_RGB_COLORS, &raw_pixels, NULL, 0);
    if (surface.bitmap == NULL || raw_pixels == NULL) {
        return false;
    }

    surface.old_bitmap = (HBITMAP)SelectObject(surface.dc, surface.bitmap);
    surface.pixels = static_cast<unsigned int*>(raw_pixels);
    return true;
}

void DestroyBitmapSurface(BitmapSurface& surface) {
    if (surface.dc != NULL && surface.old_bitmap != NULL) {
        SelectObject(surface.dc, surface.old_bitmap);
    }
    if (surface.bitmap != NULL) {
        DeleteObject(surface.bitmap);
    }
    if (surface.dc != NULL) {
        DeleteDC(surface.dc);
    }
    surface = BitmapSurface();
}

void ClearFrameBuffer(unsigned int* frame_buffer, COLORREF color) {
    const int pixel_count = width * height;
    for (int i = 0; i < pixel_count; ++i) {
        frame_buffer[i] = color;
    }
}

void ClearZBuffer(float* zbuffer) {
    const int pixel_count = width * height;
    for (int i = 0; i < pixel_count; ++i) {
        zbuffer[i] = 0.0f;
    }
}

void BlitSurface(HDC target_dc) {
    if (g_app.surface.dc == NULL) {
        return;
    }
    BitBlt(target_dc, 0, 0, width, height, g_app.surface.dc, 0, 0, SRCCOPY);
}

void PresentFrame() {
    HDC window_dc = GetDC(g_app.hwnd);
    BlitSurface(window_dc);
    ReleaseDC(g_app.hwnd, window_dc);
}

void RenderTexturedScene(Renderer& renderer) {
    for (int face = 0; face < renderer.get_nums_faces(); ++face) {
        v2f vertices[3];
        for (int i = 0; i < 3; ++i) {
            vertices[i] = renderer.vertexShader(renderer.processShader(face, i));
        }
        DrawTriangle_barycentric_Shader(vertices, g_app.scene.light_pos, renderer);
    }
}

void RenderGouraudScene(Renderer& renderer) {
    for (int face = 0; face < renderer.get_nums_faces(); ++face) {
        a2v attributes[3];
        v2f vertices[3];
        for (int i = 0; i < 3; ++i) {
            attributes[i] = renderer.processShader(face, i);
            vertices[i] = renderer.vertexShader_Gouruad(attributes[i], g_app.scene.light_pos);
        }
        DrawTriangle_barycentric_Shader_Gouruad(vertices, renderer);
    }
}

void RenderPointScene(Renderer& renderer) {
    unsigned int* frame_buffer = renderer.get_screen_fb();
    for (int face = 0; face < renderer.get_nums_faces(); ++face) {
        for (int i = 0; i < 3; ++i) {
            const v2f vertex = renderer.vertexShader(renderer.processShader(face, i));
            const int px = (int)vertex.vertex.x;
            const int py = (int)vertex.vertex.y;
            if (px < 0 || px >= width || py < 0 || py >= height) {
                continue;
            }
            frame_buffer[py * width + px] = g_app.white;
        }
    }
}

void ApplySceneInput(const ImGuiIO& io) {
    if (!io.WantCaptureKeyboard) {
        if (g_app.keys['A']) g_app.scene.camera_pos.x -= 0.2f;
        if (g_app.keys['D']) g_app.scene.camera_pos.x += 0.2f;
        if (g_app.keys['W']) g_app.scene.camera_pos.y += 0.2f;
        if (g_app.keys['S']) g_app.scene.camera_pos.y -= 0.2f;
        if (g_app.keys['Q']) g_app.scene.camera_pos.z -= 0.2f;
        if (g_app.keys['E']) g_app.scene.camera_pos.z += 0.2f;
    }

    if (!io.WantCaptureMouse && g_app.mouse_wheel_delta != 0) {
        g_app.scene.camera_pos.z -= (float)g_app.mouse_wheel_delta / (float)WHEEL_DELTA * 0.4f;
    }

    g_app.mouse_wheel_delta = 0;
}

void BuildUi() {
    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(290.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);

    if (ImGui::Begin("Renderer Control")) {
        ImGui::Text("Faces: %d", g_app.mesh != NULL ? g_app.mesh->nums_faces() : 0);
        ImGui::Text("FPS: %.1f", g_app.stats.fps);
        ImGui::Text("Frame: %.2f ms (avg %.2f ms)", g_app.stats.frame_ms, g_app.stats.avg_ms);

        if (!g_app.scene.assets_available) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "Missing obj or texture assets. Textured modes will fall back.");
        }

        const char* render_modes[] = { "Textured", "Gouraud", "Points" };
        ImGui::Combo("Mode", &g_app.scene.render_mode, render_modes, IM_ARRAYSIZE(render_modes));

        ImGui::DragFloat3("Camera", &g_app.scene.camera_pos.x, 0.05f, -20.0f, 20.0f);
        ImGui::DragFloat3("Center", &g_app.scene.center.x, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat3("Light", &g_app.scene.light_pos.x, 0.05f, -10.0f, 10.0f);
        ImGui::SliderFloat("FOV", &g_app.scene.fov, 20.0f, 90.0f);

        if (ImGui::Button("Reset View")) {
            ResetSceneState();
        }

        ImGui::Checkbox("Show Demo Window", &g_app.scene.show_demo_window);
        ImGui::Checkbox("Show Metrics Window", &g_app.scene.show_metrics_window);
        ImGui::Separator();
        ImGui::TextWrapped("W/A/S/D move the camera. Q/E and mouse wheel change depth when the UI is not capturing input.");
    }
    ImGui::End();

    if (g_app.scene.show_demo_window) {
        ImGui::ShowDemoWindow(&g_app.scene.show_demo_window);
    }
    if (g_app.scene.show_metrics_window) {
        ImGui::ShowMetricsWindow(&g_app.scene.show_metrics_window);
    }
}

void RenderScene() {
    if (g_app.renderer == NULL) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ApplySceneInput(io);

    ClearFrameBuffer(g_app.surface.pixels, g_app.background);
    ClearZBuffer(g_app.zbuffer);

    Transformer& transformer = g_app.renderer->get_transformer();
    transformer.LookAt(g_app.scene.camera_pos, g_app.scene.center, g_app.scene.up);
    transformer.perspProjection(g_app.scene.fov, g_app.scene.aspect, g_app.scene.near_plane, g_app.scene.far_plane);
    transformer.update_MVP_without_model();

    if (!g_app.scene.assets_available && g_app.scene.render_mode != RenderMode_Points) {
        RenderPointScene(*g_app.renderer);
        return;
    }

    switch (g_app.scene.render_mode) {
    case RenderMode_Gouraud:
        RenderGouraudScene(*g_app.renderer);
        break;
    case RenderMode_Points:
        RenderPointScene(*g_app.renderer);
        break;
    case RenderMode_Textured:
    default:
        RenderTexturedScene(*g_app.renderer);
        break;
    }
}

void UpdateFrameStats(double frame_start, double frame_end) {
    g_app.stats.frame_ms = (float)((frame_end - frame_start) * 1000.0);
    if (g_app.stats.sample_start == 0.0) {
        g_app.stats.sample_start = frame_end;
    }

    g_app.stats.frame_count += 1;
    const double elapsed = frame_end - g_app.stats.sample_start;
    if (elapsed >= 1.0) {
        g_app.stats.fps = (float)(g_app.stats.frame_count / elapsed);
        g_app.stats.avg_ms = (float)(elapsed * 1000.0 / g_app.stats.frame_count);
        g_app.stats.frame_count = 0;
        g_app.stats.sample_start = frame_end;
    }
}

bool InitWindow(HINSTANCE instance, int cmd_show) {
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = DefWindowProcA;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = kWindowClassName;
    window_class.lpfnWndProc = [](HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param) -> LRESULT {
        if (ImGui::GetCurrentContext() != NULL && ImGui_ImplWin32_WndProcHandler(hwnd, message, w_param, l_param)) {
            return TRUE;
        }

        switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC paint_dc = BeginPaint(hwnd, &paint);
            BlitSurface(paint_dc);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_MOUSEWHEEL:
            g_app.mouse_wheel_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            return 0;
        case WM_KEYDOWN:
            g_app.keys[w_param & 511] = 1;
            return 0;
        case WM_KEYUP:
            g_app.keys[w_param & 511] = 0;
            return 0;
        case WM_DESTROY:
            g_app.running = false;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(hwnd, message, w_param, l_param);
        }
    };

    if (!RegisterClassA(&window_class)) {
        MessageBoxA(NULL, "Window registration failed.", kWindowTitle, MB_ICONERROR);
        return false;
    }

    RECT rect = { 0, 0, width, height };
    const DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rect, window_style, FALSE);

    g_app.hwnd = CreateWindowA(
        kWindowClassName,
        kWindowTitle,
        window_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        instance,
        NULL);

    if (g_app.hwnd == NULL) {
        MessageBoxA(NULL, "Window creation failed.", kWindowTitle, MB_ICONERROR);
        return false;
    }

    if (!CreateBitmapSurface(g_app.hwnd, g_app.surface)) {
        MessageBoxA(NULL, "Bitmap surface creation failed.", kWindowTitle, MB_ICONERROR);
        DestroyWindow(g_app.hwnd);
        g_app.hwnd = NULL;
        return false;
    }

    ShowWindow(g_app.hwnd, cmd_show);
    UpdateWindow(g_app.hwnd);
    return true;
}

bool InitSceneResources() {
    g_app.scene.assets_available = FileExists(kObjPath) && FileExists(kTexPath);

    g_app.mesh = new Mesh(kObjPath, kTexPath);
    g_app.zbuffer = new float[width * height];
    g_app.image = new TGAImage(width, height, 24);

    Transformer transformer(
        g_app.scene.camera_pos,
        g_app.scene.center,
        g_app.scene.up,
        g_app.scene.fov,
        g_app.scene.aspect,
        g_app.scene.near_plane,
        g_app.scene.far_plane,
        width,
        height,
        depth,
        v_x,
        v_y);

    g_app.renderer = new Renderer(
        width,
        height,
        g_app.surface.pixels,
        g_app.zbuffer,
        g_app.mesh,
        1,
        g_app.black,
        g_app.image,
        transformer);

    ClearFrameBuffer(g_app.surface.pixels, g_app.background);
    ClearZBuffer(g_app.zbuffer);
    return true;
}

void ShutdownSceneResources() {
    delete g_app.renderer;
    g_app.renderer = NULL;

    delete g_app.image;
    g_app.image = NULL;

    delete[] g_app.zbuffer;
    g_app.zbuffer = NULL;

    delete g_app.mesh;
    g_app.mesh = NULL;
}

void InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;

    ImGui_ImplWin32_Init(g_app.hwnd);
    ImGui_SoftwareRenderer_Init();
}

void ShutdownImGui() {
    ImGui_SoftwareRenderer_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ShutdownApp() {
    if (ImGui::GetCurrentContext() != NULL) {
        ShutdownImGui();
    }
    ShutdownSceneResources();
    DestroyBitmapSurface(g_app.surface);

    if (g_app.hwnd != NULL && IsWindow(g_app.hwnd)) {
        DestroyWindow(g_app.hwnd);
        g_app.hwnd = NULL;
    }

    UnregisterClassA(kWindowClassName, GetModuleHandleA(NULL));
    FreeConsole();
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR, int cmd_show) {
    InitConsole();

    load_COLORREF(g_app.white, 255, 255, 255, 255);
    load_COLORREF(g_app.black, 0, 0, 0, 255);
    load_COLORREF(g_app.background, 128, 128, 128, 255);
    ResetSceneState();

    ImGui_ImplWin32_EnableDpiAwareness();

    if (!InitWindow(instance, cmd_show)) {
        ShutdownApp();
        return -1;
    }

    InitSceneResources();
    InitImGui();

    MSG message = {};
    while (g_app.running) {
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                g_app.running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        if (!g_app.running) {
            break;
        }

        const double frame_start = get_cpu_time();

        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        BuildUi();
        RenderScene();
        ImGui::Render();

        ImGui_SoftwareRenderer_RenderDrawData(
            ImGui::GetDrawData(),
            g_app.surface.pixels,
            width,
            height);

        PresentFrame();
        UpdateFrameStats(frame_start, get_cpu_time());
    }

    ShutdownApp();
    return 0;
}
