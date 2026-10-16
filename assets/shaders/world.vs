#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    // Meshes supply their model transform, while immediate geometry normally
    // uses identity. The fallback also tolerates an unset custom batch matrix.
    bool modelSupplied = abs(matModel[3][3]) > 0.5;
    bool normalSupplied = dot(matNormal[0].xyz, matNormal[0].xyz) > 0.0001;
    fragPosition = modelSupplied ? vec3(matModel*vec4(vertexPosition, 1.0)) : vertexPosition;
    fragNormal = normalSupplied ? mat3(matNormal)*vertexNormal : vertexNormal;
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
