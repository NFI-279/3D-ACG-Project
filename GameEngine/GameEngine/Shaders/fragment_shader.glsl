#version 400

in vec2 textureCoord;
in vec3 norm;
in vec3 fragPos;

out vec4 fragColor;

uniform sampler2D texture1;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

/* NEW */
uniform bool isTree;
uniform float trunkHeight;
uniform vec3 trunkColor;
uniform vec3 leavesColor;

void main()
{
    if (isTree)
    {
        // Height-based coloring
        vec3 color = (fragPos.y < trunkHeight)
            ? trunkColor
            : leavesColor;

        fragColor = vec4(color, 1.0);
    }
    else
    {
        // Original behavior (textures)
        fragColor = texture(texture1, textureCoord);
    }
}
