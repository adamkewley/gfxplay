#version 330 core

// this shader doesn't do anything because the fragment shader implicitly
// populates the depth buffer with the depth of each fragment that gets this
// far through the pipeline. Shadowmapping only cares about the depth buffer,
// so we're done!

void main() {
    //gl_FragDepth = gl_FragCoord.z;
}
