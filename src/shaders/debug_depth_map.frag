
in vec2 frag_texcoord;
out vec4 out_color;

// layout (binding = 0) uniform sampler2D diffuse_sampler;
layout (binding = 3) uniform sampler2DArray depth_sampler;
layout (location = 0) uniform int u_texture_index;


void main()
{

    vec2 not_fucked_texcoord = gl_FragCoord.xy / vec2(1600, 900);

    float depth_value = texture(depth_sampler, vec3(not_fucked_texcoord, u_texture_index)).r;
    out_color = vec4(vec3(depth_value), 1.0);



    //out_color = vec4(vec3(1.0f, 0.0f, 0.0f) * texcoord.x, 1.0f);

}
