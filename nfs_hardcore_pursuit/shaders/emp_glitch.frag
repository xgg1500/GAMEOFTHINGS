#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;
uniform float empIntensity;       // 0.0 to 1.0 (active during EMP detonation)
uniform vec2 shockwaveCenter;     // Screen coordinates of blast (e.g. 0.5, 0.5)

float Random(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    if (empIntensity <= 0.001) {
        FragColor = texture(screenTexture, TexCoords);
        return;
    }

    vec2 uv = TexCoords;

    // 1. Radial EMP Shockwave Distortion
    vec2 toCenter = uv - shockwaveCenter;
    float dist = length(toCenter);
    float wave = sin(dist * 35.0 - time * 18.0) * 0.035 * empIntensity;
    vec2 distortedUV = uv + normalize(toCenter + vec2(1e-4)) * wave;

    // 2. Horizontal Glitch Slices
    float sliceNoise = Random(vec2(floor(uv.y * 30.0), floor(time * 24.0)));
    if (sliceNoise > 0.82) {
        distortedUV.x += (Random(vec2(time, uv.y)) - 0.5) * 0.06 * empIntensity;
    }

    // 3. Chromatic Aberration (Prism RGB Splitting)
    float splitAmount = 0.025 * empIntensity;
    float r = texture(screenTexture, distortedUV + vec2(splitAmount, 0.0)).r;
    float g = texture(screenTexture, distortedUV).g;
    float b = texture(screenTexture, distortedUV - vec2(splitAmount, 0.0)).b;
    vec3 col = vec3(r, g, b);

    // 4. Interlaced Scanlines
    float scanline = sin(uv.y * 800.0) * 0.08 * empIntensity;
    col -= scanline;

    // 5. EMP Static Noise Flash & Inversion
    float noise = (Random(uv * time) - 0.5) * 0.25 * empIntensity;
    col += noise;

    // Cyan / Electric Blue EMP tint
    vec3 empElectricTint = vec3(0.1, 0.65, 1.2) * empIntensity * 0.35;
    col += empElectricTint;

    FragColor = vec4(col, 1.0);
}
