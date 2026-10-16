#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D texture2;
uniform sampler2D shadowMap;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform vec3 flashPos;
uniform vec3 flashDir;
uniform float flashOn;
uniform float time;
uniform float tension;
uniform float shotFlash;
// 0 procedural architecture/trim, 1 tiled photographs, 2 imported UV materials.
uniform float surfaceMode;
uniform float normalMapped;
uniform float depthPass;
uniform mat4 lightVP;
uniform float shadowsEnabled;

float hash(vec3 p)
{
    p = fract(p*0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y)*p.z);
}

float noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f*f*(3.0 - 2.0*f);
    return mix(mix(mix(hash(i), hash(i + vec3(1, 0, 0)), f.x),
                   mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
                   mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

vec3 mappedNormal(vec3 geometricNormal, vec2 uv)
{
    if (normalMapped < 0.5 || surfaceMode < 0.5) return geometricNormal;
    vec3 tangentNormal = texture(texture2, uv).xyz*2.0 - 1.0;
    // Slightly soften photographic microrelief instead of exaggerating cracks.
    tangentNormal.xy *= surfaceMode < 1.5 ? 0.72 : 0.85;
    tangentNormal = normalize(tangentNormal);
    vec3 dpdx = dFdx(fragPosition);
    vec3 dpdy = dFdy(fragPosition);
    vec2 duvdx = dFdx(uv);
    vec2 duvdy = dFdy(uv);
    vec3 perpendicularY = cross(dpdy, geometricNormal);
    vec3 perpendicularX = cross(geometricNormal, dpdx);
    vec3 tangent = perpendicularY*duvdx.x + perpendicularX*duvdy.x;
    vec3 bitangent = perpendicularY*duvdx.y + perpendicularX*duvdy.y;
    float frameSize = max(dot(tangent, tangent), dot(bitangent, bitangent));
    if (frameSize < 0.00000001) return geometricNormal;
    float inverseSize = inversesqrt(frameSize);
    return normalize(mat3(tangent*inverseSize, bitangent*inverseSize, geometricNormal)*tangentNormal);
}

float flashlightVisibility(vec3 geometricNormal, vec3 lightDirection)
{
    if (shadowsEnabled < 0.5) return 1.0;
    vec4 projected = lightVP*vec4(fragPosition, 1.0);
    if (projected.w <= 0.0) return 1.0;
    vec3 coordinate = projected.xyz/projected.w*0.5 + 0.5;
    // A flashlight is not an infinitely repeated texture outside its frustum.
    if (coordinate.z <= 0.0 || coordinate.z >= 1.0 ||
        coordinate.x <= 0.0 || coordinate.x >= 1.0 ||
        coordinate.y <= 0.0 || coordinate.y >= 1.0) return 1.0;
    float bias = max(0.00008*(1.0 - max(dot(geometricNormal, lightDirection), 0.0)), 0.00002);
    vec2 texel = 1.0/vec2(textureSize(shadowMap, 0));
    float visible = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 sampleUv = clamp(coordinate.xy + vec2(x, y)*texel, texel*0.5, 1.0 - texel*0.5);
            float closestDepth = texture(shadowMap, sampleUv).r;
            visible += coordinate.z - bias <= closestDepth ? 1.0 : 0.0;
        }
    }
    return visible/9.0;
}

vec3 specularResponse(vec3 n, vec3 viewDirection, vec3 lightDirection, float roughness, vec3 reflectance)
{
    vec3 halfVector = normalize(viewDirection + lightDirection);
    float nDotL = max(dot(n, lightDirection), 0.0);
    float nDotV = max(dot(n, viewDirection), 0.0);
    float exponent = mix(100.0, 9.0, roughness*roughness);
    float highlight = pow(max(dot(n, halfVector), 0.0), exponent);
    vec3 fresnel = reflectance + (1.0 - reflectance)*pow(1.0 - nDotV, 5.0);
    return fresnel*highlight*nDotL*(0.35 + (1.0 - roughness)*0.65);
}

void main()
{
    if (depthPass > 0.5) {
        finalColor = vec4(1.0);
        return; // Rasterization writes the real geometry depth automatically.
    }

    vec3 geometricNormal = length(fragNormal) > 0.01 ? normalize(fragNormal)
        : normalize(cross(dFdx(fragPosition), dFdy(fragPosition)));
    vec3 an = abs(geometricNormal);
    bool floorSurface = an.y > 0.6;
    vec2 surface = floorSurface ? fragPosition.xz
        : (an.x > 0.6 ? fragPosition.zy : fragPosition.xy);
    vec2 uv = surfaceMode > 0.5 && surfaceMode < 1.5 ? surface*0.52 : fragTexCoord;
    vec3 n = mappedNormal(geometricNormal, uv);
    vec4 texel = texture(texture0, uv);
    float alpha = texel.a*colDiffuse.a*fragColor.a;
    if (alpha < 0.05) discard;

    float largeNoise = noise(fragPosition*0.72);
    vec3 displayBase = texel.rgb*colDiffuse.rgb*fragColor.rgb;
    // Imported glTF color factors are already linear; photographic color maps
    // and draw-command tints use sRGB. Keeping this separate preserves assets.
    vec3 base = surfaceMode > 1.5
        ? pow(max(texel.rgb, vec3(0.0)), vec3(2.2))*colDiffuse.rgb*pow(fragColor.rgb, vec3(2.2))
        : pow(max(displayBase, vec3(0.0)), vec3(2.2));

    if (surfaceMode < 0.5) {
        float smallNoise = noise(fragPosition*11.0);
        float grit = hash(floor(fragPosition*95.0));
        float concrete = 0.76 + 0.17*largeNoise + 0.06*smallNoise + 0.025*grit;
        vec2 panelUv = surface*(floorSurface ? vec2(1.0) : vec2(0.50, 0.34));
        vec2 edgeDistance = min(fract(panelUv), 1.0 - fract(panelUv));
        vec2 width = max(fwidth(panelUv)*1.2, vec2(0.0035));
        float seam = 1.0 - min(smoothstep(0.0, width.x, edgeDistance.x),
                               smoothstep(0.0, width.y, edgeDistance.y));
        base *= concrete*(1.0 - seam*0.18);
    } else if (surfaceMode < 1.5) {
        // Albedo and normal photographs provide the detail; retain their range.
        base *= 0.94 + largeNoise*0.09;
    }

    vec3 toCamera = viewPos - fragPosition;
    float cameraDistance = max(length(toCamera), 0.001);
    vec3 viewDirection = toCamera/cameraDistance;
    float roughness = surfaceMode > 1.5 ? 0.43
        : (floorSurface ? mix(0.48, 0.66, largeNoise) : 0.83);
    vec3 reflectance = surfaceMode > 1.5 ? vec3(0.075, 0.078, 0.075) : vec3(0.035);
    vec3 ambient = vec3(0.105, 0.120, 0.115)*(0.88 + 0.12*geometricNormal.y);
    // An unobtrusive hemisphere fill keeps unlit rooms navigable.
    vec3 color = base*ambient;

    for (int i = 0; i < 7; ++i) {
        vec3 lampPosition = vec3(0.0, 3.55, 10.6 - float(i)*5.4);
        vec3 lightOffset = lampPosition - fragPosition;
        float distanceSquared = dot(lightOffset, lightOffset);
        float distanceToLamp = sqrt(max(distanceSquared, 0.0001));
        vec3 lightDirection = lightOffset/distanceToLamp;
        float rangeFactor = clamp(1.0 - distanceSquared*distanceSquared/256.0, 0.0, 1.0);
        float attenuation = rangeFactor*rangeFactor/(1.0 + 0.18*distanceSquared);
        vec3 lampColor = i%3 == 1 ? vec3(1.0, 0.63, 0.31) : vec3(0.63, 0.85, 0.80);
        float flicker = 1.0;
        // Match visible fixture flicker, with no abrupt whole-screen flashes.
        if (i%3 == 0 && sin(time*9.0 + float(i))*sin(time*17.0) < -0.5) flicker = 0.32;
        vec3 radiance = lampColor*(i%3 == 1 ? 4.0 : 3.3)*attenuation*flicker;
        float nDotL = max(dot(n, lightDirection), 0.0);
        color += radiance*(base*nDotL + specularResponse(n, viewDirection, lightDirection, roughness, reflectance));
    }

    vec3 flashlightOffset = flashPos - fragPosition;
    float flashDistance = max(length(flashlightOffset), 0.001);
    vec3 flashlightDirection = flashlightOffset/flashDistance;
    vec3 beamDirection = length(flashDir) > 0.01 ? normalize(flashDir) : vec3(0.0, 0.0, -1.0);
    float cone = dot(-flashlightDirection, beamDirection);
    float outerCone = smoothstep(0.775, 0.935, cone);
    float innerCone = smoothstep(0.92, 0.991, cone);
    float flashlightFalloff = 1.0/(1.0 + flashDistance*flashDistance*0.035);
    float lampFlicker = 0.992 + 0.008*sin(time*48.0 + tension*2.0);
    float visibility = flashlightVisibility(geometricNormal, flashlightDirection);
    vec3 flashlightRadiance = vec3(1.0, 0.965, 0.83)*max(flashOn, 0.0)*lampFlicker
        *(outerCone*1.35 + innerCone*1.75)*flashlightFalloff*visibility;
    color += flashlightRadiance*(base*max(dot(n, flashlightDirection), 0.0)
        + specularResponse(n, viewDirection, flashlightDirection, roughness, reflectance));

    vec3 muzzleRadiance = vec3(1.0, 0.43, 0.10)*max(shotFlash, 0.0)*3.7
        /(1.0 + cameraDistance*cameraDistance*0.17);
    color += base*muzzleRadiance*(0.3 + 0.7*max(dot(n, viewDirection), 0.0));

    float fog = 1.0 - exp(-max(cameraDistance - 4.0, 0.0)*(0.021 + tension*0.006));
    color = mix(color, vec3(0.010, 0.017, 0.017), clamp(fog, 0.0, 0.80));
    // Gentle photographic compression preserves lit concrete without crushing
    // the detail in dark rooms; the post shader adds only restrained film grain.
    color *= 1.20;
    color = color/(vec3(0.85) + color);
    color = pow(max(color, vec3(0.0)), vec3(1.0/2.2));
    finalColor = vec4(clamp(color, 0.0, 1.0), alpha);
}
