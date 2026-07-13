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

static PosColorVertex cubeVertices[] = {
    {-0.5f,  0.5f,  0.5f, 0xff0000ff }, // 0: front-top-left    (red)
    { 0.5f,  0.5f,  0.5f, 0xff00ff00 }, // 1: front-top-right   (green)
    {-0.5f, -0.5f,  0.5f, 0xffff0000 }, // 2: front-bottom-left (blue)
    { 0.5f, -0.5f,  0.5f, 0xffffff00 }, // 3: front-bottom-right(yellow)
    {-0.5f,  0.5f, -0.5f, 0xffff00ff }, // 4: back-top-left     (magenta)
    { 0.5f,  0.5f, -0.5f, 0xff00ffff }, // 5: back-top-right    (cyan)
    {-0.5f, -0.5f, -0.5f, 0xffffffff }, // 6: back-bottom-left  (white)
    { 0.5f, -0.5f, -0.5f, 0xff000000 }, // 7: back-bottom-right (black)
};

static const uint16_t cubeIndices[] = {
    0, 1, 2,  1, 3, 2,  // front face
    4, 6, 5,  5, 6, 7,  // back face
    4, 0, 6,  0, 2, 6,  // left face
    1, 5, 3,  5, 7, 3,  // right face
    4, 5, 0,  5, 1, 0,  // top face
    2, 3, 6,  3, 7, 6,  // bottom face
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

    // Camera state
    float camPos[3] = { 0.0f, 0.0f, -3.0f };
    float camYaw = 0.0f;   // left-right rotation
    float camPitch = 0.0f; // up-down rotation
    float camSpeed = 3.0f; // units per second

    SDL_SetRelativeMouseMode(SDL_TRUE); // mouse cursor hide + lock kar

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, 1280, 720);

    // Vertex layout define kar (bgfx ko batana ki data kaise arranged hai)
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), layout
    );

    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(cubeIndices, sizeof(cubeIndices))
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
            if (e.type == SDL_MOUSEMOTION) {
                float sensitivity = 0.003f;
                camYaw += e.motion.xrel * sensitivity;
                camPitch -= e.motion.yrel * sensitivity;

                // Pitch ko limit kar (upar-neeche zyada na ghoome)
                if (camPitch > 1.5f) camPitch = 1.5f;
                if (camPitch < -1.5f) camPitch = -1.5f;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        // Delta time nikaal (frame ke beech ka time)
        static uint32_t lastTime = SDL_GetTicks();
        uint32_t currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Keyboard input se movement
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

        // Camera "look at" point nikaal (yaw/pitch se)
        float lookX = camPos[0] + bx::sin(camYaw) * bx::cos(camPitch);
        float lookY = camPos[1] + bx::sin(camPitch);
        float lookZ = camPos[2] + bx::cos(camYaw) * bx::cos(camPitch);

        float view[16];
        float proj[16];
        bx::mtxLookAt(view, {camPos[0], camPos[1], camPos[2]}, {lookX, lookY, lookZ});
        bx::mtxProj(proj, 60.0f, 1280.0f / 720.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);

        bgfx::touch(0);

        // Cube ab static rahega apni jagah (rotate nahi karega, camera hi ghoomega)
        float mtx[16];
        bx::mtxIdentity(mtx);
        bgfx::setTransform(mtx);

        // Cube draw kar
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(0, program);

        bgfx::frame();
    }

    bgfx::destroy(vbh);
    bgfx::destroy(ibh);
    bgfx::destroy(program);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}