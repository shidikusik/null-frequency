#pragma once
#include "raylib.h"
#include "rlgl.h"

// Depth-only spotlight shadow target. FBO ownership follows raylib's shadowmap
// example (raylib 5.5, zlib license); the world shader filters it with 3x3 PCF.
struct ShadowMap {
    RenderTexture2D target{};
    bool load(int size=2048) {
        target.id=rlLoadFramebuffer();
        target.texture.width=size; target.texture.height=size;
        if(!target.id) return false;
        rlEnableFramebuffer(target.id);
        target.depth.id=rlLoadTextureDepth(size,size,false);
        target.depth.width=size; target.depth.height=size;
        target.depth.format=19; target.depth.mipmaps=1;
        rlFramebufferAttach(target.id,target.depth.id,RL_ATTACHMENT_DEPTH,RL_ATTACHMENT_TEXTURE2D,0);
        bool complete=rlFramebufferComplete(target.id);
        rlDisableFramebuffer();
        SetTextureWrap(target.depth,TEXTURE_WRAP_CLAMP);
        SetTextureFilter(target.depth,TEXTURE_FILTER_POINT);
        return complete;
    }
    void bind(Shader shader) const {
        rlEnableShader(shader.id); rlActiveTextureSlot(10); rlEnableTexture(target.depth.id);
        int unit=10; rlSetUniform(GetShaderLocation(shader,"shadowMap"),&unit,SHADER_UNIFORM_INT,1);
        rlActiveTextureSlot(0);
    }
    void unbind() const { rlActiveTextureSlot(10); rlDisableTexture(); rlActiveTextureSlot(0); }
    void unload() { if(target.id) rlUnloadFramebuffer(target.id); target={}; }
};
