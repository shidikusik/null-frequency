#version 330
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;
void main() {
    vec3 n = normalize(fragNormal);
    vec3 light = normalize(vec3(-0.5, 0.8, 0.7));
    vec4 base = texture(texture0, fragTexCoord)*colDiffuse*fragColor;
    float diffuse = max(dot(n, light), 0.0);
    float sheen = pow(max(dot(reflect(-light, n), normalize(-fragPosition)),0.0),24.0);
    float rim = pow(1.0-max(dot(n,normalize(-fragPosition)),0.0),3.0);
    vec3 color = base.rgb*(0.52+diffuse*1.12) + vec3(0.21,0.24,0.23)*sheen;
    color += vec3(0.035,0.052,0.055)*rim;
    finalColor = vec4(color, base.a);
}
