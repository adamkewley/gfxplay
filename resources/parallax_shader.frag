#version 330 core

in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

out vec4 FragColor;

uniform sampler2D DiffuseTex;
uniform sampler2D NormalTex;
uniform sampler2D DepthTex;
uniform float HeightScale;

vec2 ParallaxedTexCoords(vec3 viewDir) {
    // basic parallax mapping (not this):
    //
    // - sample texture at original texture coordinate
    // - based on the depth (higher depths have a higher effect), linearly
    //   offset the original texture coordinate away from the viewer (in
    //   tangent space),
    // - this yields a texture coordinate that is linearly offset by the
    //   depth. When the offset textel is rendered, it gives the illusion of
    //   depth
    //
    // basic parallax mapping is simple and works well when the viewer views
    // the surface face-on (or at slight angles). However, it breaks down when
    // the viewer views the surface at very shallow angles because the linear
    // offsetting technique doesn't necessarily yield the "correct" textel that
    // true depth would yield (imagine viewing a very rough, brick wall at a
    // shallow angle - this technique might offset a textel by *a lot* from
    // some nook in the cement and yield a texel that is several bricks over
    // in the wall, which looks *very* wrong).

    // steep parallax mapping (this)
    //
    // - split depth [0.0 - 1.0], into equally-sized layers
    // - produce new (offset) texture coord foreach layer (e.g. try
    //   original texture coord, then original texture coord + LAYER, then
    //   original texture coord + 2*LAYER, etc.)
    // - for each coord, sample the depth texture
    // - if sampled depth < layer depth, then the current offset coordinate is
    //   *probably* representitive of what "real" parallax mapping would yield
    //   (e.g. if the "real" surface is a steep, but narrow, ditch and we
    //   used "basic" mapping (above) we might end up with coordinates that are
    //   far away from where they really should be. Sampling at multiple points
    //   takes the "shape" of the surface into account (slightly)

    // use more samples if viewing from a shallow angle, because depth is more
    // pronounced at those angles (the offset vectors are bigger)
    const float min_layers = 8.0f;
    const float max_layers = 32.0f;
    float num_layers = mix(max_layers, min_layers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));
    float layer_depth = 1.0f/num_layers;

    float cur_layer_depth = 0.0f;
    vec2 P = viewDir.xy/viewDir.z * HeightScale;
    vec2 dP = P/num_layers;
    vec2 coords = TexCoords;
    float sampled_depth = texture(DepthTex, coords).r;

    while (cur_layer_depth < sampled_depth) {
        coords -= dP;
        sampled_depth = texture(DepthTex, coords).r;
        cur_layer_depth += layer_depth;
    }

    // parallax occlusion mapping: linearly interpolate either side of the
    // sample coords to get a more accurate coordinate value
    vec2 prev_coords = coords + dP;
    float dDepth_after = sampled_depth - cur_layer_depth;
    float dDepth_before = texture(DepthTex, prev_coords).r - (cur_layer_depth - layer_depth);
    float weight = dDepth_after/(dDepth_after - dDepth_before);
    vec2 final_coords = prev_coords*weight + coords*(1.0f - weight);

    return final_coords;
}

void main() {
    // parallax mapping: texture coordinates are computed per-fragment to
    // offset them based on the height from the displacement texture
    vec3 tangentViewDir = normalize(TangentViewPos - TangentFragPos);
    vec2 parallaxedTexCoords = ParallaxedTexCoords(tangentViewDir);

    // the post-parallax-mapping texture coords might fall outside of the
    // texture's coordinates which, depending on the texture's wrapping mode,
    // can give bizzare-looking results
    //
    // here, we just discard any texture coordinate that falls outside the usual
    // range so that the shader isn't dependent on certain wrapping modes
    {
        bool parallax_coord_fell_outside_texture =
            parallaxedTexCoords.x > 1.0
            || parallaxedTexCoords.y > 1.0
            || parallaxedTexCoords.x < 0.0
            || parallaxedTexCoords.y < 0.0;

        if (parallax_coord_fell_outside_texture) {
            discard;
        }
    }

    vec3 color = texture(DiffuseTex, parallaxedTexCoords).rgb;

    // normal map is in tangent space and in the range [0,1]
    // remap to [-1,1] (so we can go "down" as well as "up")
    vec3 normal = texture(NormalTex, parallaxedTexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    // lighting calculations are performed in tangent space

    vec3 tangentLightDir = normalize(TangentLightPos - TangentFragPos);
    vec3 lightColor = vec3(2.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;

    // diffuse
    float diff = max(dot(tangentLightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular (Blinn-Phong)
    vec3 tangentReflectDir = reflect(-tangentLightDir, normal);
    vec3 tangentHalfwayDir = normalize(tangentLightDir + tangentViewDir);
    float spec = pow(max(dot(normal, tangentHalfwayDir), 0.0), 32.0);

    vec3 specular = spec * 0.1f * lightColor; // assuming bright white light color

    float distance = length(TangentLightPos - TangentFragPos);
    float attenuation = 1.0 / (distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;

    FragColor = vec4((ambient + diffuse + specular) * color, 1.0);
}
