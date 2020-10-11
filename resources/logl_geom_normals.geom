#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

const float MAGNITUDE = 0.3f;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void GenerateLine(int index) {
    // emit original vertex as-is
    gl_Position = projection * view * model * gl_in[index].gl_Position;
    EmitVertex();

    mat4 normalMatrix = mat4(mat3(transpose(inverse(model))));
    vec4 normalVec = normalize(projection * view * normalMatrix * vec4(gs_in[index].normal, 0.0f));
    normalVec *= MAGNITUDE;
    gl_Position = (projection * view * model * gl_in[index].gl_Position) + normalVec;
    EmitVertex();

    // which results in a 2-vertex line primitive
    EndPrimitive();
}

void main() {
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}
