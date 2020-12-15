#version 330 core

// Blinn-Phong + shadowmap shader

in vec3 Normal;
in vec2 TexCoord;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float far_plane;

#define NUM_SAMPLES 20
// sampling Disk Radius (DR): effectively, offsets from a the depthmap sampling
// direction
#define DR 0.05f

// LUT of shadow sampling directions.
const vec3 sampleOffsetDirections[NUM_SAMPLES] = vec3[](
    vec3( DR,  DR,  DR),
    vec3( DR, -DR,  DR),
    vec3(-DR, -DR,  DR),
    vec3(-DR,  DR,  DR),

    vec3( DR,  DR, -DR),
    vec3( DR, -DR, -DR),
    vec3(-DR, -DR, -DR),
    vec3(-DR,  DR, -DR),

    vec3( DR,  DR,  0),
    vec3( DR, -DR,  0),
    vec3(-DR, -DR,  0),
    vec3(-DR,  DR,  0),

    vec3( DR,  0,  DR),
    vec3(-DR,  0,  DR),
    vec3( DR,  0, -DR),
    vec3(-DR,  0, -DR),

    vec3( 0,  DR,  DR),
    vec3( 0, -DR,  DR),
    vec3( 0, -DR, -DR),
    vec3( 0,  DR, -DR)
);

float ShadowStrength(vec3 lightToFrag) {
    float currentDepth = length(lightToFrag);
    float bias   = 0.15;
    float biasedCurrentDepth = currentDepth - bias;

    float shadow = 0.0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float closestDepth = texture(depthMap, lightToFrag + sampleOffsetDirections[i]).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if (biasedCurrentDepth > closestDepth) {
            shadow += 1.0;
        }
    }
    shadow /= float(NUM_SAMPLES);
    return shadow;
}

void main(void) {
    vec3 diffuseColor = texture(diffuseTexture, TexCoord).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(4.0f);
    vec3 fragToLight = lightPos - FragPos;
    vec3 fragToLightDir = normalize(fragToLight);
    vec3 lightToFrag = -fragToLight;
    vec3 fragToViewDir = normalize(viewPos - FragPos);

    vec3 ambient = vec3(0.02f);

    float diffuseAmt = max(dot(fragToLightDir, normal), 0.0f);
    vec3 diffuse = vec3(diffuseAmt);

    vec3 halfwayDir = normalize(fragToLightDir - fragToViewDir);
    float specularAmt = pow(max(dot(normal, halfwayDir), 0.0f), 64.0f);
    vec3 specular = vec3(specularAmt);

    // attenuate the light: only affects diffuse + specular
    float distance = length(lightToFrag);
    float attenuation = 1.0f / (distance*distance);

    // calculate shadows: only affects diffuse + specular
    float shadow = ShadowStrength(lightToFrag);

    vec3 lighting = (ambient + (1.0f - shadow)*attenuation*(diffuse + specular)) * diffuseColor * lightColor;

    FragColor = vec4(lighting, 1.0f);
}
