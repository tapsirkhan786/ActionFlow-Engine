#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <iostream>
#include <fstream>
#include <vector>

// Vertex structure: position + color
struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
};

static PosColorVertex triangleVertices[] = {
    { 0.0f,  0.5f, 0.0f, 0xff0000ff }, // top (red)
    { 0.5f, -0.5f, 0.0f, 0xff00ff00 }, // bottom-right (green)
    {-0.5f, -0.5f, 0.0f, 0xffff0000 }, // bottom-left (blue)
};

// Shader file ko memory me load karne ka helper function
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

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "ActionFlow Engine - Day 1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        std::cout << "Window could not be created! Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        std::cout << "Could not get window info! Error: " << SDL_GetError() << std::endl;
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

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, 1280, 720);

    // Vertex layout define kar (bgfx ko batana ki data kaise arranged hai)
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(triangleVertices, sizeof(triangleVertices)), layout
    );

    // Shaders load kar aur program banao
    bgfx::ShaderHandle vsh = loadShader("shaders/vs_triangle.bin");
    bgfx::ShaderHandle fsh = loadShader("shaders/fs_triangle.bin");
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        // Camera aur projection set kar (simple 2D-facing view)
        float view[16];
        float proj[16];
        bx::mtxLookAt(view, {0.0f, 0.0f, -3.0f}, {0.0f, 0.0f, 0.0f});
        bx::mtxProj(proj, 60.0f, 1280.0f / 720.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);

        bgfx::touch(0);

        // Triangle draw kar
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(0, program);

        bgfx::frame();
    }

    bgfx::destroy(vbh);
    bgfx::destroy(program);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}