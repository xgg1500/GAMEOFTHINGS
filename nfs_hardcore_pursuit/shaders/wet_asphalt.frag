#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 cameraPos;
uniform sampler2D asphaltTexture;
uniform sampler2D puddleMask;
uniform samplerCube envMap;
uniform float wetnessAmount;       // 0.0 (bone dry) to 1.0 (torrential rain)

// Active thermite burn scorches on asphalt
uniform vec3 thermitePos[8];
uniform float thermiteRadii[8];
uniform int activeThermiteCount;

void main() {
    vec3 N = vec3(0.0, 1.0, 0.0); // Road flat normal
    vec3 V = normalize(cameraPos - FragPos);
    vec3 R = reflect(-V, N);

    // Procedural asphalt stone grain
    float stoneNoise = fract(sin(dot(FragPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
    vec3 baseAlbedo = vec3(0.12, 0.12, 0.13) + (stoneNoise * 0.02);

    // Puddle distribution
    float puddle = smoothstep(0.45, 0.65, sin(FragPos.x * 0.15) * cos(FragPos.z * 0.15) + wetnessAmount * 0.5);
    float roughness = mix(0.85, 0.04, puddle * wetnessAmount);

    // Environment reflection on wet surfaces
    vec3 skyReflection = texture(envMap, R).rgb;
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = 0.04 + 0.96 * pow(1.0 - NdotV, 5.0);

    vec3 wetColor = mix(baseAlbedo * 0.6, skyReflection, fresnel * (1.0 - roughness));

    // Check for molten thermite burn patches
    vec3 thermiteGlow = vec3(0.0);
    for (int i = 0; i < activeThermiteCount; ++i) {
        float dist = length(FragPos.xz - thermitePos[i].xz);
        if (dist < thermiteRadii[i]) {
            float heat = 1.0 - (dist / thermiteRadii[i]);
            // Incandescent 2500°C molten core (white-hot to fiery orange)
            vec3 coreColor = mix(vec3(1.0, 0.4, 0.05), vec3(1.5, 1.2, 0.8), pow(heat, 3.0));
            thermiteGlow += coreColor * heat * 4.0;
        }
    }

    vec3 finalColor = wetColor + thermiteGlow;
    FragColor = vec4(finalColor, 1.0);
}
