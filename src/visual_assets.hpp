#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// Downloaded meshes keep their original topology, materials and skeletal clips.
// Ownership stays here: raylib's UnloadModel intentionally does not free textures.
struct VisualAssets {
    struct Asset { Model model{}; BoundingBox bounds{}; bool ready=false; };
    Asset weapon, utility, barrel, chair;
    std::vector<Asset> figures;
    ModelAnimation* clips=nullptr;
    int clipCount=0, walkClip=0, idleClip=0, attackClip=0, deathClip=-1;
    std::vector<float> deathAt;
    std::vector<int> lastFrame,lastClip;
    Texture2D wallTexture{},floorTexture{},wallNormal{},floorNormal{};
    Model wallCube{},floorCube{};
    std::set<unsigned int> ownedTextures;

    Asset loadModel(const char* path,Shader shader) {
        Asset a;
        if(!FileExists(path)) { std::cerr<<"Missing model: "<<path<<"\n"; return a; }
        a.model=LoadModel(path); a.ready=IsModelValid(a.model);
        if(!a.ready) return a;
        a.bounds=GetModelBoundingBox(a.model);
        for(int m=0;m<a.model.materialCount;++m) {
            a.model.materials[m].shader=shader;
            for(int map=0;map<=MATERIAL_MAP_BRDF;++map) {
                auto texture=a.model.materials[m].maps[map].texture;
                if(texture.id>1) ownedTextures.insert(texture.id);
            }
        }
        return a;
    }
    bool load(Shader world,Shader gun) {
        weapon=loadModel("assets/models/weapon/weapon.glb",gun);
        if(weapon.ready) for(int i=0;i<weapon.model.materialCount;++i) {
            auto& c=weapon.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
            auto srgb=[](unsigned char x){return static_cast<unsigned char>(255*std::pow(x/255.f,1.f/2.2f));};
            c={srgb(c.r),srgb(c.g),srgb(c.b),c.a};
        }
        utility=loadModel("assets/models/props/utility/utility.gltf",world);
        barrel=loadModel("assets/models/props/barrel/barrel.gltf",world);
        chair=loadModel("assets/models/props/chair/chair.gltf",world);
        if(FileExists("assets/models/entity/entity.glb")) {
            clips=LoadModelAnimations("assets/models/entity/entity.glb",&clipCount);
            for(int i=0;i<7;++i) figures.push_back(loadModel("assets/models/entity/entity.glb",world));
            deathAt.assign(7,-1); lastFrame.assign(7,-1); lastClip.assign(7,-1);
            for(int i=0;i<clipCount;++i) {
                std::string name=clips[i].name;
                std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                if(name.find("walk")!=std::string::npos) walkClip=i;
                if(name.find("idle")!=std::string::npos) idleClip=i;
                if(name.find("attack")!=std::string::npos||name.find("punch")!=std::string::npos) attackClip=i;
                if(name.find("death")!=std::string::npos) deathClip=i;
                if(name=="walk_loop") walkClip=i;
                if(name=="idle_loop") idleClip=i;
            }
            std::cout<<"Imported animated entity: "<<clipCount<<" clips, walk="<<walkClip<<", idle="<<idleClip<<", attack="<<attackClip<<"\n";
        }
        wallTexture=LoadTexture("assets/textures/wall.jpg");
        floorTexture=LoadTexture("assets/textures/floor.jpg");
        wallNormal=LoadTexture("assets/textures/wall-normal.jpg");
        floorNormal=LoadTexture("assets/textures/floor-normal.jpg");
        for(auto t:{wallTexture,floorTexture,wallNormal,floorNormal}) if(t.id) {
            SetTextureWrap(t,TEXTURE_WRAP_REPEAT); SetTextureFilter(t,TEXTURE_FILTER_ANISOTROPIC_8X); ownedTextures.insert(t.id);
        }
        wallCube=LoadModelFromMesh(GenMeshCube(1,1,1)); floorCube=LoadModelFromMesh(GenMeshCube(1,1,1));
        wallCube.materials[0].shader=world; floorCube.materials[0].shader=world;
        wallCube.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=wallTexture;
        floorCube.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=floorTexture;
        wallCube.materials[0].maps[MATERIAL_MAP_NORMAL].texture=wallNormal;
        floorCube.materials[0].maps[MATERIAL_MAP_NORMAL].texture=floorNormal;
        bool animated=figures.size()==7&&clips&&clipCount>0;
        for(const auto& figure:figures) {
            animated=animated&&figure.ready;
            if(figure.ready) for(int i=0;i<clipCount;++i) animated=animated&&IsModelAnimationValid(figure.model,clips[i]);
        }
        return weapon.ready&&utility.ready&&barrel.ready&&chair.ready&&animated&&
               wallTexture.id&&floorTexture.id&&wallNormal.id&&floorNormal.id;
    }
    static void draw(const Asset& asset,Vector3 ground,float height,float angle=0,Color tint=WHITE) {
        if(!asset.ready) return;
        float normal=0;
        for(int i=0;i<asset.model.materialCount;++i) if(asset.model.materials[i].maps[MATERIAL_MAP_NORMAL].texture.id>1) normal=1;
        rlDrawRenderBatchActive();
        auto shader=asset.model.materials[0].shader;
        SetShaderValue(shader,GetShaderLocation(shader,"normalMapped"),&normal,SHADER_UNIFORM_FLOAT);
        float scale=height/std::max(.001f,asset.bounds.max.y-asset.bounds.min.y);
        Vector3 center{(asset.bounds.min.x+asset.bounds.max.x)*.5f,asset.bounds.min.y,(asset.bounds.min.z+asset.bounds.max.z)*.5f};
        center=Vector3RotateByAxisAngle(center,{0,1,0},angle*DEG2RAD);
        ground=Vector3Subtract(ground,Vector3Scale(center,scale));
        DrawModelEx(asset.model,ground,{0,1,0},angle,{scale,scale,scale},tint);
    }
    void figure(std::size_t index,const nf::Enemy& enemy,nf::Vec3 player,float clock) {
        if(index>=figures.size()||!figures[index].ready) return;
        if(enemy.alive) deathAt[index]=-1;
        else if(enemy.kind||deathClip<0) return;
        else if(deathAt[index]<0) deathAt[index]=clock;
        if(deathAt[index]>=0&&clock-deathAt[index]>7) return;
        auto& asset=figures[index];
        int selected=enemy.kind?idleClip:(enemy.attackCooldown>.45f?attackClip:enemy.seenTime>0?walkClip:idleClip);
        if(!enemy.alive) selected=deathClip;
        if(clips&&selected<clipCount&&IsModelAnimationValid(asset.model,clips[selected])) {
            int frame=enemy.alive?static_cast<int>((clock+enemy.phase)*1000.f/17.f)%std::max(1,clips[selected].frameCount)
                                 :std::min(clips[selected].frameCount-1,static_cast<int>((clock-deathAt[index])*1000.f/17.f));
            if(frame!=lastFrame[index]||selected!=lastClip[index]) {
                UpdateModelAnimation(asset.model,clips[selected],frame); lastFrame[index]=frame; lastClip[index]=selected;
            }
        }
        float angle=std::atan2(player.x-enemy.position.x,player.z-enemy.position.z)*RAD2DEG;
        draw(asset,{enemy.position.x,enemy.position.y,enemy.position.z},enemy.kind?2.05f:1.96f,angle,
             enemy.kind?Color{95,137,134,255}:Color{128,132,123,255});
    }
    void unload() {
        auto release=[](Asset& a){if(a.ready) UnloadModel(a.model); a.ready=false;};
        release(weapon);release(utility);release(barrel);release(chair);
        for(auto& a:figures) release(a);
        if(clips) UnloadModelAnimations(clips,clipCount);
        if(wallCube.meshCount) UnloadModel(wallCube);
        if(floorCube.meshCount) UnloadModel(floorCube);
        for(auto id:ownedTextures) UnloadTexture(Texture2D{id,0,0,0,0});
        ownedTextures.clear();
    }
};
