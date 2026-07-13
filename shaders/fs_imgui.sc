$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

uniform sampler2D s_tex;

void main() {
    vec4 texel = texture2D(s_tex, v_texcoord0);
    gl_FragColor = v_color0 * texel;
}