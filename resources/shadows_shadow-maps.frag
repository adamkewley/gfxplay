#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords*0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // bias: removes shadow ache
    //
    // because the depth map is sampled at a different (probably, lower)
    // resolution than the (currently redered) screen, a phenomenon called
    // "shadow achne" can occur (see: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)
    //
    // this happens when the light is at an angle to the surface. In that case,
    // the depth-mapping step is going to probe at some resolution (of the depth
    // map) and assign a depth to each fragment *in the depth map*.
    //
    // When this fragment shader computes fragments from exactly the same
    // perspective, it will be doing so at a different resolution. This means that
    // the depth of each fragment (from light's perspective) will be slightly
    // lower than the value pulled from the depth map.
    //
    // applying a bias helps remove this affect. Try setting this bias to 0 to
    // see what happens (artifacts, where the light is angled)
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // sampling with percentage-closer filtering (PCF)
    //
    // because the depth map might be a lower resolution than what's currently
    // being rendered, the resulting shadow can appear "blocky".
    //
    // The solution to this is to use PCF to sample the depth map multiple
    // times. Instead of sampling it in one location (coord.xy), we sample a
    // 3x3 square in which the central square is what we would've sampled and
    // the outer (8) squares are the nearest 8 neighbours in the depth map.
    //
    // note: we need to know the size of a single textel in the shadowmap so
    // that this sampling process samples the neighbouring textel, rather than
    // just sampling (probably) the same textel over and over.
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 textelCoord = projCoords.xy + vec2(x, y)*texelSize;
            float pcfDepth = texture(shadowMap, textelCoord).r;

            // this is the same as without PCFing the samples
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // if the current fragment's Z coordinate (in light space) is >1 then the
    // fragment was outside of the light's projection frustrum (ortho). In this
    // case, there is no shadow (it's "out of view" and looks weird to assume
    // it is in shadow there)
    shadow = projCoords.z > 1.0 ? 0.0 : shadow;

    return shadow;
}

void main() {
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);

    // ambient
    vec3 ambient = 0.3 * color;

    // diffuse
    vec3 diffuse = max(dot(lightDir, normal), 0.0) * lightColor;

    // specular
    //
    // Blinn-Phong alg: find the halfway vector between the light and view.
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.0);
}

