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
    // deep = white, shallow = black
    float depth =  texture(DepthTex, TexCoords).r;

    // because view direction is in tangent space, its Y aligns with U, X aligns
    // with V and Z aligns with N.
    //
    // so xy/z scales the UV coordinate based on how far the view is "above"
    // the surface. e.g. if it was face-on
    //
    // leaving the Z division out is called "Parallax Mapping with Offset Limiting"
    vec2 p = (viewDir.xy/viewDir.z) * (depth * HeightScale);
    return TexCoords - p;
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
    vec3 lightColor = vec3(5.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;

    // diffuse
    float diff = max(dot(tangentLightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular (Blinn-Phong)
    vec3 tangentReflectDir = reflect(-tangentLightDir, normal);
    vec3 tangentHalfwayDir = normalize(tangentLightDir + tangentViewDir);
    float spec = pow(max(dot(normal, tangentHalfwayDir), 0.0), 64.0);

    vec3 specular = spec * lightColor; // assuming bright white light color

    float distance = length(TangentLightPos - TangentFragPos);
    float attenuation = 1.0 / (distance * distance);
    //diffuse *= attenuation;
    //specular *= attenuation;

    FragColor = vec4((ambient + diffuse + specular) * color, 1.0);
}
