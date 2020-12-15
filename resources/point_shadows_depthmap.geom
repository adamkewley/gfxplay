#version 330 core

// Because a cubemap has 6 faces, each input triangle (3 verts) produces 6
// output triangles in lightspace (18 verts)

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

// Worldspace location of lightspace output vert
out vec4 FragPos;

// Worldspace-to-lightspace matrix
uniform mat4 shadowMatrices[6];

#define VERTS_IN_TRIANGLE 3

void main() {
    for (int face = 0; face < 6; ++face) {
        // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl_Layer.xhtml
        //
        // defines which layer (or face of a cubemap) this primitive will render
        // to (e.g. 0 == GL_TEXTURE_CUBEMAP_POSITIVE_X)
        gl_Layer = face;

        for (int vert = 0; vert < VERTS_IN_TRIANGLE; ++vert) {
            FragPos = gl_in[vert].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}
