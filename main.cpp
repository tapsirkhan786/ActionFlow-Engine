#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <imgui.h>
#include "vs_ocornut_imgui.bin.h"
#include "fs_ocornut_imgui.bin.h"
#include <iostream>
#include <fstream>
#include <vector>

// Vertex structure: position + color
struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
};

static PosColorVertex cubeVertices[] = {
    {-0.5f,  0.5f,  0.5f, 0xff0000ff },
    { 0.5f,  0.5f,  0.5f, 0xff00ff00 },
    {-0.5f, -0.5f,  0.5f, 0xffff0000 },
    { 0.5f, -0.5f,  0.5f, 0xffffff00 },
    {-0.5f,  0.5f, -0.5f, 0xffff00ff },
    { 0.5f,  0.5f, -0.5f, 0xff00ffff },
    {-0.5f, -0.5f, -0.5f, 0xffffffff },
    { 0.5f, -0.5f, -0.5f, 0xff000000 },
};

static const uint16_t cubeIndices[] = {
    0, 1, 2,  1, 3, 2,
    4, 6, 5,  5, 6, 7,
    4, 0, 6,  0, 2, 6,
    1, 5, 3,  5, 7, 3,
    4, 5, 0,  5, 1, 0,
    2, 3, 6,  3, 7, 6,
};

static bgfx::ShaderHandle loadShader(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "Could not open shader file: " << filename << std::endl;
        exit(1);
    }
    size_t size = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);
    const bgfx::Memory* mem = bgfx::alloc((uint32_t)size + 1);
    file.read((char*)mem->data, size);
    mem->data[size] = '\0';
    return bgfx::createShader(mem);
}

// Helper: SDL scancode -> ImGuiKey mapping
static ImGuiKey sdlKeycodeToImGui(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_TAB: return ImGuiKey_Tab;
        case SDLK_LEFT: return ImGuiKey_LeftArrow;
        case SDLK_RIGHT: return ImGuiKey_RightArrow;
        case SDLK_UP: return ImGuiKey_UpArrow;
        case SDLK_DOWN: return ImGuiKey_DownArrow;
        case SDLK_PAGEUP: return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
        case SDLK_HOME: return ImGuiKey_Home;
        case SDLK_END: return ImGuiKey_End;
        case SDLK_INSERT: return ImGuiKey_Insert;
        case SDLK_DELETE: return ImGuiKey_Delete;
        case SDLK_BACKSPACE: return ImGuiKey_Backspace;
        case SDLK_SPACE: return ImGuiKey_Space;
        case SDLK_RETURN: return ImGuiKey_Enter;
        case SDLK_ESCAPE: return ImGuiKey_Escape;
        case SDLK_QUOTE: return ImGuiKey_Apostrophe;
        case SDLK_COMMA: return ImGuiKey_Comma;
        case SDLK_MINUS: return ImGuiKey_Minus;
        case SDLK_PERIOD: return ImGuiKey_Period;
        case SDLK_SLASH: return ImGuiKey_Slash;
        case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
        case SDLK_EQUALS: return ImGuiKey_Equal;
        case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
        case SDLK_BACKSLASH: return ImGuiKey_Backslash;
        case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
        case SDLK_BACKQUOTE: return ImGuiKey_GraveAccent;
        case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
        case SDLK_PAUSE: return ImGuiKey_Pause;
        case SDLK_F1: return ImGuiKey_F1;
        case SDLK_F2: return ImGuiKey_F2;
        case SDLK_F3: return ImGuiKey_F3;
        case SDLK_F4: return ImGuiKey_F4;
        case SDLK_F5: return ImGuiKey_F5;
        case SDLK_F6: return ImGuiKey_F6;
        case SDLK_F7: return ImGuiKey_F7;
        case SDLK_F8: return ImGuiKey_F8;
        case SDLK_F9: return ImGuiKey_F9;
        case SDLK_F10: return ImGuiKey_F10;
        case SDLK_F11: return ImGuiKey_F11;
        case SDLK_F12: return ImGuiKey_F12;
        default: break;
    }
    if (sym >= SDLK_a && sym <= SDLK_z) {
        return (ImGuiKey)(ImGuiKey_A + (sym - SDLK_a));
    }
    if (sym >= SDLK_0 && sym <= SDLK_9) {
        return (ImGuiKey)(ImGuiKey_0 + (sym - SDLK_0));
    }
    return ImGuiKey_None;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "ActionFlow Engine - ImGui + 3D",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720, SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cout << "Window could not be created!" << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        std::cout << "Could not get window info!" << std::endl;
        return 1;
    }

    bgfx::PlatformData pd{};
    pd.nwh = wmi.info.win.window;

    bgfx::Init init;
    init.platformData = pd;
    init.type = bgfx::RendererType::Direct3D11;
    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        std::cout << "bgfx could not initialize!" << std::endl;
        return 1;
    }

    // ===== ImGui Init =====
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    ImGui::StyleColorsDark();
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    // Font atlas -> bgfx texture
    unsigned char* pixels;
    int fontW, fontH;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fontW, &fontH);

    bgfx::TextureHandle fontTex = bgfx::createTexture2D(
        (uint16_t)fontW, (uint16_t)fontH, false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE,
        bgfx::copy(pixels, fontW * fontH * 4)
    );

    bgfx::UniformHandle imguiSampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    // ImGui program
    const bgfx::Memory* vsMem = bgfx::makeRef(vs_ocornut_imgui_dxbc, sizeof(vs_ocornut_imgui_dxbc));
    const bgfx::Memory* fsMem = bgfx::makeRef(fs_ocornut_imgui_dxbc, sizeof(fs_ocornut_imgui_dxbc));
    bgfx::ShaderHandle imguiVsh = bgfx::createShader(vsMem);
    bgfx::ShaderHandle imguiFsh = bgfx::createShader(fsMem);
    bgfx::ProgramHandle imguiProgram = bgfx::createProgram(imguiVsh, imguiFsh, true);

    // Vertex layout for ImGui (position + color, no texcoord in our simple shader)
    bgfx::VertexLayout imguiLayout;
imguiLayout.begin()
    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

    // ===== Camera state =====
    float camPos[3] = { 0.0f, 0.0f, -3.0f };
    float camYaw = 0.0f;
    float camPitch = 0.0f;
    float camSpeed = 3.0f;
    bool mouseLocked = false;

    // ===== Cube setup =====
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, 1280, 720);

    bgfx::VertexLayout cubeLayout;
    cubeLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), cubeLayout
    );
    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(cubeIndices, sizeof(cubeIndices))
    );

    bgfx::ShaderHandle vsh = loadShader("shaders/vs_triangle.bin");
    bgfx::ShaderHandle fsh = loadShader("shaders/fs_triangle.bin");
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    // ===== Main loop =====
    bool running = true;
    bool showUI = true;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }

            // Camera mouse look (only when mouse locked, not over ImGui)
            if (e.type == SDL_MOUSEMOTION) {
                if (mouseLocked && !io.WantCaptureMouse) {
                    float sensitivity = 0.003f;
                    camYaw += e.motion.xrel * sensitivity;
                    camPitch -= e.motion.yrel * sensitivity;
                    if (camPitch > 1.5f) camPitch = 1.5f;
                    if (camPitch < -1.5f) camPitch = -1.5f;
                }
            }

            // Forward events to ImGui
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                io.AddMouseButtonEvent(e.button.button - 1, true);
            }
            if (e.type == SDL_MOUSEBUTTONUP) {
                io.AddMouseButtonEvent(e.button.button - 1, false);
            }
            if (e.type == SDL_MOUSEWHEEL) {
                io.AddMouseWheelEvent((float)e.wheel.x, (float)e.wheel.y);
            }

            if (e.type == SDL_KEYDOWN) {
                // Modifiers
                SDL_Keycode sym = e.key.keysym.sym;
                if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) io.AddKeyEvent(ImGuiKey_LeftCtrl, true);
                if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) io.AddKeyEvent(ImGuiKey_LeftShift, true);
                if (sym == SDLK_LALT || sym == SDLK_RALT) io.AddKeyEvent(ImGuiKey_LeftAlt, true);
                if (sym == SDLK_LGUI || sym == SDLK_RGUI) io.AddKeyEvent(ImGuiKey_LeftSuper, true);

                ImGuiKey key = sdlKeycodeToImGui(sym);
                if (key != ImGuiKey_None) {
                    io.AddKeyEvent(key, true);
                    io.SetKeyEventNativeData(key, e.key.keysym.sym, e.key.keysym.scancode, e.key.keysym.scancode);
                }
            }
            if (e.type == SDL_KEYUP) {
                SDL_Keycode sym = e.key.keysym.sym;
                if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) io.AddKeyEvent(ImGuiKey_LeftCtrl, false);
                if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) io.AddKeyEvent(ImGuiKey_LeftShift, false);
                if (sym == SDLK_LALT || sym == SDLK_RALT) io.AddKeyEvent(ImGuiKey_LeftAlt, false);
                if (sym == SDLK_LGUI || sym == SDLK_RGUI) io.AddKeyEvent(ImGuiKey_LeftSuper, false);

                ImGuiKey key = sdlKeycodeToImGui(sym);
                if (key != ImGuiKey_None) {
                    io.AddKeyEvent(key, false);
                }
            }
            if (e.type == SDL_TEXTINPUT) {
                io.AddInputCharactersUTF8(e.text.text);
            }
        }

        // ===== Update =====
        static uint32_t lastTime = SDL_GetTicks();
        uint32_t currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (mouseLocked) {
            const Uint8* keys = SDL_GetKeyboardState(NULL);
            float forward[3] = { bx::sin(camYaw), 0.0f, bx::cos(camYaw) };
            float right[3]   = { bx::cos(camYaw), 0.0f, -bx::sin(camYaw) };

            if (keys[SDL_SCANCODE_W]) {
                camPos[0] += forward[0] * camSpeed * dt;
                camPos[2] += forward[2] * camSpeed * dt;
            }
            if (keys[SDL_SCANCODE_S]) {
                camPos[0] -= forward[0] * camSpeed * dt;
                camPos[2] -= forward[2] * camSpeed * dt;
            }
            if (keys[SDL_SCANCODE_A]) {
                camPos[0] -= right[0] * camSpeed * dt;
                camPos[2] -= right[2] * camSpeed * dt;
            }
            if (keys[SDL_SCANCODE_D]) {
                camPos[0] += right[0] * camSpeed * dt;
                camPos[2] += right[2] * camSpeed * dt;
            }
            if (keys[SDL_SCANCODE_SPACE]) camPos[1] += camSpeed * dt;
            if (keys[SDL_SCANCODE_LCTRL]) camPos[1] -= camSpeed * dt;
        }

        // Mouse position update
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        io.AddMousePosEvent((float)mx, (float)my);

        // ===== Start ImGui frame =====
        ImGui::NewFrame();

        if (showUI) {
            ImGui::Begin("ActionFlow Engine - Control Panel", &showUI);
            ImGui::Text("Welcome to your game engine!");
            ImGui::Separator();
            ImGui::Text("Camera Position: %.2f, %.2f, %.2f", camPos[0], camPos[1], camPos[2]);
            ImGui::Text("Camera Yaw/Pitch: %.2f / %.2f", camYaw, camPitch);
            ImGui::Separator();
            ImGui::Text("Mouse: %s", mouseLocked ? "LOCKED (FPS mode)" : "FREE (UI mode)");
            if (ImGui::Button(mouseLocked ? "Unlock Mouse (use UI)" : "Lock Mouse (FPS mode)")) {
                mouseLocked = !mouseLocked;
                SDL_SetRelativeMouseMode(mouseLocked ? SDL_TRUE : SDL_FALSE);
            }
            ImGui::SliderFloat("Camera Speed", &camSpeed, 1.0f, 10.0f);
            ImGui::Text("Color Picker: Coming Soon");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::End();
        }

        // ===== 3D scene =====
        float lookX = camPos[0] + bx::sin(camYaw) * bx::cos(camPitch);
        float lookY = camPos[1] + bx::sin(camPitch);
        float lookZ = camPos[2] + bx::cos(camYaw) * bx::cos(camPitch);

        float view[16], proj[16];
        bx::mtxLookAt(view, {camPos[0], camPos[1], camPos[2]}, {lookX, lookY, lookZ});
        bx::mtxProj(proj, 60.0f, 1280.0f / 720.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);
        bgfx::setViewRect(0, 0, 0, 1280, 720);

        bgfx::touch(0);

        float mtx[16];
        bx::mtxIdentity(mtx);
        bgfx::setTransform(mtx);
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(0, program);

        // ===== ImGui render =====
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData && drawData->CmdListsCount > 0) {
            float ortho[16];
            bx::mtxOrtho(ortho, 0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
            bgfx::setViewTransform(1, nullptr, ortho);
            bgfx::setViewRect(1, 0, 0, 1280, 720);

            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList* cmdList = drawData->CmdLists[n];

                bgfx::TransientVertexBuffer tvb;
                bgfx::TransientIndexBuffer tib;
                uint32_t vtxSize = (uint32_t)cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
                uint32_t idxSize = (uint32_t)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);

                bgfx::allocTransientVertexBuffer(&tvb, cmdList->VtxBuffer.Size, imguiLayout);
                bgfx::allocTransientIndexBuffer(&tib, cmdList->IdxBuffer.Size);

                memcpy(tvb.data, cmdList->VtxBuffer.Data, vtxSize);
                memcpy(tib.data, cmdList->IdxBuffer.Data, idxSize);

                for (int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++) {
                    const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];
                    if (pcmd->UserCallback) {
                        pcmd->UserCallback(cmdList, pcmd);
                        continue;
                    }

                    uint16_t scissorX = (uint16_t)pcmd->ClipRect.x;
                    uint16_t scissorY = (uint16_t)pcmd->ClipRect.y;
                    uint16_t scissorW = (uint16_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                    uint16_t scissorH = (uint16_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                    bgfx::setScissor(scissorX, scissorY, scissorW, scissorH);

                    bgfx::setTexture(0, imguiSampler, fontTex);
                    bgfx::setVertexBuffer(0, &tvb);
                    bgfx::setIndexBuffer(&tib, pcmd->IdxOffset, pcmd->ElemCount);
                    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA, 0);
                    bgfx::submit(1, imguiProgram);
                }
            }
        }

        bgfx::frame();
    }

    // ===== Cleanup =====
    bgfx::destroy(vbh);
    bgfx::destroy(ibh);
    bgfx::destroy(program);
    bgfx::destroy(imguiProgram);
    bgfx::destroy(imguiSampler);
    bgfx::destroy(fontTex);
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}