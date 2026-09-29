#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneColor;
uniform float speedKmh;           // Dynamic speed vignette & camera vibration
uniform float exposure;           // HDR camera exposure
uniform float vignetteStrength;

// ACES Filmic Tone Mapping (Narkowicz 2015 approximation)
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec2 uv = TexCoords;

    // Sample HDR Scene color
    vec3 hdrColor = texture(sceneColor, uv).rgb;

    // Apply exposure
    hdrColor *= exposure;

    // High-speed tunnel vision vignette
    float speedRatio = clamp(speedKmh / 320.0, 0.0, 1.0);
    vec2 centerOffset = uv - vec2(0.5);
    float vignette = 1.0 - dot(centerOffset, centerOffset) * (1.2 + speedRatio * 1.5);
    vignette = clamp(vignette, 0.0, 1.0);

    // Apply ACES Tone mapping
    vec3 ldrColor = ACESFilm(hdrColor);

    // Vignette falloff
    ldrColor *= vignette;

    // Gamma correction (sRGB space)
    ldrColor = pow(ldrColor, vec3(1.0 / 2.2));

    FragColor = vec4(ldrColor, 1.0);
}
