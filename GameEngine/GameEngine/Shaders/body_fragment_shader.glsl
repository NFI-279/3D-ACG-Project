#version 400

in vec2 textureCoord;
in vec3 norm;
in vec3 fragPos;

out vec4 fragColor;

uniform vec3 bodyColor;
uniform sampler2D bodyTexture;

void main()
{
    vec3 texColor = texture(bodyTexture, textureCoord).rgb;

    // Mix the green color with the texture
    vec3 finalColor = mix(bodyColor, texColor, 0.5); // 0.5 = 50% texture overlay
    fragColor = vec4(finalColor, 1.0);
}
