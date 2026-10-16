#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform float tension;
uniform float damage;
uniform float glitched;
uniform float grain;

float random(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7)))*43758.5453);
}

void main()
{
    vec2 screen = max(resolution, vec2(1.0));
    vec2 uv = fragTexCoord;
    float stress = clamp(tension, 0.0, 1.0);
    float glitchAmount = clamp(glitched, 0.0, 1.0);
    float tick = floor(time*16.0);
    float band = step(0.982, random(vec2(floor(uv.y*52.0), tick)));
    uv.x += band*(random(vec2(tick, floor(uv.y*31.0))) - 0.5)*0.016*glitchAmount;
    uv = clamp(uv, vec2(0.001), vec2(0.999));

    vec2 direction = uv - 0.5;
    vec2 aberration = direction*(0.18 + stress*1.2 + glitchAmount*1.1)/screen;
    vec3 color;
    color.r = texture(texture0, clamp(uv + aberration, vec2(0.001), vec2(0.999))).r;
    color.g = texture(texture0, uv).g;
    color.b = texture(texture0, clamp(uv - aberration, vec2(0.001), vec2(0.999))).b;

    float edge = smoothstep(0.15, 0.74, length(direction*vec2(1.08, 1.0)));
    color *= 1.0 - edge*(0.19 + stress*0.075);
    float scan = sin(uv.y*screen.y*3.14159265)*0.003;
    color += vec3(scan*clamp(grain, 0.0, 1.0));
    float film = random(uv*screen + vec2(floor(time*30.0), 0.0)) - 0.5;
    color += film*(0.014 + stress*0.010)*clamp(grain, 0.0, 1.0);
    color += vec3(0.009, 0.0, 0.005)*band*glitchAmount;
    float damageEdge = clamp(damage, 0.0, 1.0)*edge;
    color = mix(color, color*vec3(0.72, 0.40, 0.38) + vec3(0.18, 0.006, 0.0), damageEdge*0.65);
    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0)*fragColor;
}
