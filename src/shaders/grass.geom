layout (triangles) in;
layout (line_strip, max_vertices = 12) out;

layout (location = 88) uniform float elapsedTime;

in VS_OUT
{
  vec3 normal;
} gs_in[];

const float MAGNITUDE = 2.5;

void GenerateGrassBlade(vec4 position)
{
    gl_Position = position;
    EmitVertex();
    vec3 direction = gs_in[0].normal;
    direction.x += 0.1f * cos(elapsedTime);
    direction.z += 0.1f * sin(elapsedTime);
    direction = normalize(direction);
    direction *= MAGNITUDE;


    gl_Position = position + vec4(direction, 0.0f);
    EmitVertex();
    EndPrimitive();
}


void main()
{

    // float offsetX = gl_in[0].gl_Position.x - gl_in[0].gl_Position.x;

    GenerateGrassBlade(gl_in[0].gl_Position);
    GenerateGrassBlade(gl_in[0].gl_Position + vec4(0.1f, 0.0f, 0.0f, 0.0f));
    GenerateGrassBlade(gl_in[1].gl_Position);
    GenerateGrassBlade(gl_in[2].gl_Position);

    //GenerateLine(0);
    //GenerateLine(1);
    //GenerateLine(2);


    // int xend = density;
    // for (int z = 0; z < density; z++) {
    //     for (int x = 0; x < xend; x++) {
    //         GenerateGrassBlade(gl_in[0].gl_Position.xyz);
    //     }
    //     xend--;
    // }
}
