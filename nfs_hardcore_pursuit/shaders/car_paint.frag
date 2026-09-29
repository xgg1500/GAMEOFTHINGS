#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 Tangent;
in vec3 Bitangent;
in vec4 FragPosLightSpace;

// Material properties
uniform vec3 baseColor;          // Car paint base pigment (e.g. Midnight Metallic Blue or Candy Red)
uniform float metallic;          // 0.0 to 1.0 (High for metallic paint)
uniform float roughness;         // Base coat roughness
uniform float clearCoat;         // Clear coat layer intensity (1.0 for glossy lacquer)
uniform float clearCoatRoughness;// Clear coat roughness (usually 0.05 for wet-look shine)
uniform float flakeIntensity;    // Metallic flake sparkle intensity

// Camera and Environment
uniform vec3 cameraPos;
uniform samplerCube envMap;
uniform sampler2D shadowMap;

// Dynamic Police Emergency Lights (Red & Blue Flashers)
uniform vec3 policeLightRedPos;
uniform vec3 policeLightBluePos;
uniform float policeStrobeIntensity; // Pulsing 0.0 to 1.0

const float PI = 3.14159265359;

// GGX / Trowbridge-Reitz Normal Distribution Function
float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-5);
}

// Smith GGX Geometric Shadowing
float GeometrySchlickGGX(float NdotV, float rough) {
    float r = (rough + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, rough);
    float ggx1 = GeometrySchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}

// Fresnel Schlick approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Metallic Flake Sparkle pseudo-noise
float FlakeSparkle(vec3 pos, vec3 N, vec3 V) {
    vec3 p = fract(pos * 80.0) - 0.5;
    float d = dot(p, p);
    float sparkle = smoothstep(0.12, 0.0, d);
    float viewAngle = max(dot(N, V), 0.0);
    return sparkle * viewAngle * flakeIntensity;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 R = reflect(-V, N);

    // F0 reflectance at normal incidence (dielectric 0.04 vs conductor metallic)
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    // Base directional streetlight / moon light
    vec3 lightDir = normalize(vec3(0.4, 0.9, 0.3));
    vec3 lightColor = vec3(1.4, 1.35, 1.3);

    // Cook-Torrance BRDF for main light
    vec3 H = normalize(V + lightDir);
    float NdotL = max(dot(N, lightDir), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, lightDir, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);

    // Flake sparkle modulation
    float sparkle = FlakeSparkle(FragPos, N, V);
    vec3 diffuseColor = (baseColor + vec3(sparkle)) / PI;

    vec3 directLighting = (kD * diffuseColor + specular) * lightColor * NdotL;

    // Dual-Layer Polyurethane Clear-Coat Reflection
    // Clear-coat has its own glass-like IOR (~1.5 -> F0 = 0.04)
    vec3 H_cc = normalize(V + lightDir);
    float NDF_cc = DistributionGGX(N, H_cc, clearCoatRoughness);
    float G_cc   = GeometrySmith(N, V, lightDir, clearCoatRoughness);
    vec3  F_cc   = FresnelSchlick(max(dot(H_cc, V), 0.0), vec3(0.04));
    vec3 clearCoatSpec = (NDF_cc * G_cc * F_cc) / (4.0 * NdotV * NdotL + 0.0001);

    // Apply clear-coat layer on top of base paint
    vec3 finalColor = directLighting * (1.0 - clearCoat * F_cc.r) + (clearCoatSpec * clearCoat);

    // Dynamic Police Strobe Lights Illumination
    // Red Flasher Light
    vec3 toRedLight = policeLightRedPos - FragPos;
    float distRed = length(toRedLight);
    vec3 L_red = normalize(toRedLight);
    float attenRed = 1.0 / (1.0 + 0.08 * distRed + 0.03 * distRed * distRed);
    vec3 H_red = normalize(V + L_red);
    float NDF_red = DistributionGGX(N, H_red, clearCoatRoughness);
    vec3 redSpecular = vec3(1.0, 0.05, 0.05) * NDF_red * max(dot(N, L_red), 0.0) * attenRed * policeStrobeIntensity * 3.5;

    // Blue Flasher Light
    vec3 toBlueLight = policeLightBluePos - FragPos;
    float distBlue = length(toBlueLight);
    vec3 L_blue = normalize(toBlueLight);
    float attenBlue = 1.0 / (1.0 + 0.08 * distBlue + 0.03 * distBlue * distBlue);
    vec3 H_blue = normalize(V + L_blue);
    float NDF_blue = DistributionGGX(N, H_blue, clearCoatRoughness);
    vec3 blueSpecular = vec3(0.05, 0.35, 1.0) * NDF_blue * max(dot(N, L_blue), 0.0) * attenBlue * (1.0 - policeStrobeIntensity) * 3.5;

    finalColor += redSpecular + blueSpecular;

    // Ambient environment reflection
    vec3 envReflection = texture(envMap, R).rgb * 0.45;
    finalColor += envReflection * mix(F0, vec3(1.0), pow(1.0 - NdotV, 4.0));

    FragColor = vec4(finalColor, 1.0);
}
