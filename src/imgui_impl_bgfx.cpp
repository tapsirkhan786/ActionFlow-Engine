#include "imgui_impl_bgfx.h"
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <vector>

namespace ImGui::BGFX {

static int g_ViewId = 0;
static bgfx::ProgramHandle g_Program = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle g_AttribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::TextureHandle g_FontTexture = BGFX_INVALID_HANDLE;
static bgfx::VertexBufferHandle g_VB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_IB = BGFX_INVALID_HANDLE;

// Shaders (compiled D3D11 HLSL — same as bgfx examples)
static const char* s_VertexShader = R"(
struct VS_OUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

VS_OUT main(float2 pos : POSITION, float2 uv : TEXCOORD0, uint vertexColor : COLOR0) {
    VS_OUT out;
    out.position = float4(pos, 0.0f, 1.0f);
    out.texCoord = uv;
    out.color = unpack_rgba(vertexColor);
    return out;
}
)";

static const char* s_FragmentShader = R"(
sampler2D s_AttribLocationTex;

float4 main(float2 uv : TEXCOORD0, float4 color : COLOR0) : SV_TARGET {
    return color * texture2D(s_AttribLocationTex, uv);
}
)";

static bgfx::ShaderHandle compileShader(const char* source, const char* type) {
    bgfx::Memory* mem = bgfx::allocEmbedded(nullptr, 0);
    std::vector<uint8_t> buffer;

    // Use bgfx's internal compiler (requires shaderc runtime)
    // For simplicity, we'll use bgfx::createShader with pre-compiled bytes
    // But since we have shaderc.exe, we can compile on the fly
    // For now, return invalid - we'll use a different approach
    (void)source; (void)type;
    return BGFX_INVALID_HANDLE;
}

bool Init(int viewId) {
    g_ViewId = viewId;

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_bgfx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // Build font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    g_FontTexture = bgfx::createTexture2D(
        (uint16_t)width, (uint16_t)height, false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE,
        bgfx::copy(pixels, width * height * 4)
    );

    // Create vertex layout
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    (void)layout; // stored implicitly in draw

    // Use bgfx's built-in ImGui-ready shaders (bgfx ships with these)
    // We embed them as pre-compiled blobs via bgfx_shader.sh output
    // For now, we'll use a runtime compilation approach

    g_AttribLocationTex = bgfx::createUniform("s_AttribLocationTex", bgfx::UniformType::Sampler);

    return true;
}

void Shutdown() {
    if (bgfx::isValid(g_VB)) bgfx::destroy(g_VB);
    if (bgfx::isValid(g_IB)) bgfx::destroy(g_IB);
    if (bgfx::isValid(g_FontTexture)) bgfx::destroy(g_FontTexture);
    if (bgfx::isValid(g_Program)) bgfx::destroy(g_Program);
    if (bgfx::isValid(g_AttribLocationTex)) bgfx::destroy(g_AttribLocationTex);
}

void NewFrame() {
    // nothing to do
}

void RenderDrawData(ImDrawData* drawData) {
    if (!drawData || drawData->CmdListsCount == 0) return;

    ImGuiIO& io = ImGui::GetIO();
    int fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fbWidth <= 0 || fbHeight <= 0) return;

    drawData->ScaleClipRects(io.DisplayFramebufferScale);

    // Orthographic projection matrix
    float ortho[16];
    bx::mtxOrtho(ortho, 0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, 1.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewTransform(g_ViewId, nullptr, ortho);
    bgfx::setViewRect(g_ViewId, 0, 0, (uint16_t)fbWidth, (uint16_t)fbHeight);

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];

        // Update vertex/index buffers
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;
        bgfx::allocTransientVertexBuffer(&tvb, cmdList->VtxBuffer.Size, imguiGetVertexLayout());
        bgfx::allocTransientIndexBuffer(&tib, cmdList->IdxBuffer.Size);

        ImDrawVert* verts = (ImDrawVert*)tvb.data;
        for (int i = 0; i < cmdList->VtxBuffer.Size; i++) {
            verts[i].pos[0] = cmdList->VtxBuffer[i].pos.x;
            verts[i].pos[1] = cmdList->VtxBuffer[i].pos.y;
            verts[i].uv[0]  = cmdList->VtxBuffer[i].uv.x;
            verts[i].uv[1]  = cmdList->VtxBuffer[i].uv.y;
            verts[i].col    = cmdList->VtxBuffer[i].col;
        }

        memcpy(tib.data, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

        for (int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];

            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmdList, pcmd);
            } else {
                bgfx::setScissor(g_ViewId,
                    (uint16_t)pcmd->ClipRect.x,
                    (uint16_t)pcmd->ClipRect.y,
                    (uint16_t)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                    (uint16_t)(pcmd->ClipRect.w - pcmd->ClipRect.y));

                bgfx::TextureHandle tex = (bgfx::TextureHandle)pcmd->GetTexID();
                if (!bgfx::isValid(tex)) tex = g_FontTexture;
                bgfx::setTexture(0, g_AttribLocationTex, tex);

                bgfx::setVertexBuffer(0, &tvb, cmdList->VtxBuffer.Size);
                bgfx::setIndexBuffer(&tib, pcmd->IdxOffset, pcmd->ElemCount);
                bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_FUNC_NORMAL, 0);
                bgfx::submit(g_ViewId, imguiGetProgram());
            }
        }
    }
}

// Forward declarations for shader/vertex-layout accessors (defined in imgui_impl_bgfx_shaders.cpp)
extern const bgfx::VertexLayout& imguiGetVertexLayout();
extern bgfx::ProgramHandle imguiGetProgram();

} // namespace