#include "core.hpp"
#include "audio.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "visual_assets.hpp"
#include "shadow_map.hpp"
#include "weapon_view.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr float PI_F = 3.14159265359f;
const Color ink{9, 16, 19, 255}, paper{225, 225, 209, 255};
const Color muted{132, 153, 149, 255}, signal{223, 77, 56, 255}, mint{113, 206, 188, 255};
Vector3 v(nf::Vec3 p) { return {p.x, p.y, p.z}; }
float saturate(float x) { return std::clamp(x, 0.0f, 1.0f); }
float mix(float a, float b, float t) { return a + (b-a)*t; }
void cube(Vector3 p, Vector3 s, Color c) { DrawCubeV(p, s, c); }

struct Presentation {
    Font ui{}, title{};
    Shader world{}, post{}, weaponShader{};
    RenderTexture2D scene{}, weaponLayer{};
    std::vector<Texture2D> signs;
    VisualAssets models;
    ShadowMap shadows;
    nf::WeaponView weaponView;
    int sceneWidth = 0, sceneHeight = 0;
    bool reduced = false, mutedAudio = false;
    float clock = 0, recoil = 0, walkPhase = 0, aim = 0;
    float uiScale = 1;
    Vector2 uiOffset{};

    bool load() {
        std::vector<int> codes;
        for (int c=32; c<383; ++c) codes.push_back(c);
        for (int c=0x400; c<=0x45f; ++c) codes.push_back(c);
        for (int c : {0x2013,0x2014,0x2018,0x2019,0x2022,0x2116}) codes.push_back(c);
        ui = LoadFontEx("assets/fonts/NotoSans.ttf", 38, codes.data(), static_cast<int>(codes.size()));
        title = LoadFontEx("assets/fonts/Oswald.ttf", 150, codes.data(), static_cast<int>(codes.size()));
        if (!IsFontValid(ui) || !IsFontValid(title)) return false;
        SetTextureFilter(ui.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(title.texture, TEXTURE_FILTER_BILINEAR);
        world = LoadShader("assets/shaders/world.vs", "assets/shaders/world.fs");
        world.locs[SHADER_LOC_MAP_NORMAL]=GetShaderLocation(world,"texture2");
        post = LoadShader(nullptr, "assets/shaders/post.fs");
        weaponShader = LoadShader("assets/shaders/world.vs", "assets/shaders/weapon.fs");
        // A failed custom shader can return raylib's default shader; verify custom uniforms too.
        if (!IsShaderValid(world) || !IsShaderValid(post) || !IsShaderValid(weaponShader) ||
            GetShaderLocation(world,"viewPos") < 0 || GetShaderLocation(post,"tension") < 0) return false;
        if(!models.load(world,weaponShader)) return false;
        if(!shadows.load()) return false;
        if(!weaponView.load(weaponShader)) return false;
        makeSigns();
        resize();
        return IsRenderTextureValid(scene);
    }
    void resize() {
        int w=std::max(64,GetScreenWidth()), h=std::max(64,GetScreenHeight());
        if (w==sceneWidth && h==sceneHeight) return;
        if (scene.id) UnloadRenderTexture(scene);
        if (weaponLayer.id) UnloadRenderTexture(weaponLayer);
        scene=LoadRenderTexture(w,h); sceneWidth=w; sceneHeight=h;
        weaponLayer=LoadRenderTexture(w,h);
        SetTextureFilter(scene.texture,TEXTURE_FILTER_BILINEAR);
        uiScale=std::min(w/1440.0f,h/900.0f);
        uiOffset={(w-1440*uiScale)*.5f,(h-900*uiScale)*.5f};
    }
    void text(const std::string& s,float x,float y,float size,Color c=paper,float spacing=0.7f) const {
        DrawTextEx(ui,s.c_str(),{x,y},size,spacing,c);
    }
    void heading(const std::string& s,float x,float y,float size,Color c=paper,float spacing=2) const {
        DrawTextEx(title,s.c_str(),{x,y},size,spacing,c);
    }
    void centered(const std::string& s,float y,float size,Color c=paper) const {
        float w=MeasureTextEx(ui,s.c_str(),size,.7f).x;
        text(s,(1440-w)*.5f,y,size,c);
    }
    void beginUI() const {
        Camera2D camera{uiOffset,{0,0},0,uiScale};
        BeginMode2D(camera);
    }
    Vector2 mouseUI() const {
        auto m=GetMousePosition(); return {(m.x-uiOffset.x)/uiScale,(m.y-uiOffset.y)/uiScale};
    }
    bool button(const std::string& label,Rectangle r,bool primary=false) const {
        bool hover=CheckCollisionPointRec(mouseUI(),r);
        DrawRectangleRec(r,primary ? (hover ? Color{218,88,62,255}:signal) : (hover?Color{39,57,58,245}:Color{18,30,34,235}));
        if (!primary) DrawRectangleLinesEx(r,1,Fade(muted,hover?.65f:.22f));
        text(label,r.x+22,r.y+(r.height-24)*.5f,24,primary?ink:paper,1);
        text(hover?">":"+",r.x+r.width-38,r.y+(r.height-24)*.5f,24,primary?ink:muted);
        return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }
    void makeSigns() {
        const std::array<const char*,7> labels={"01 / ARCHIVE","02 / OBSERVATION","03 / RESONANCE","04 / CONTROL","05 / TRANSMISSION","06 / SILENCE","NULL / 00"};
        for (auto label:labels) {
            auto rt=LoadRenderTexture(768,160);
            BeginTextureMode(rt);
            ClearBackground(Color{18,29,31,255});
            DrawRectangle(0,0,12,160,signal);
            DrawTextEx(title,label,{35,20},75,2,paper);
            DrawTextEx(ui,"ACOUSTIC RESEARCH / AUTHORIZED PERSONNEL",{36,117},17,2,muted);
            EndTextureMode();
            Image image=LoadImageFromTexture(rt.texture);
            ImageFlipVertical(&image);
            signs.push_back(LoadTextureFromImage(image));
            UnloadImage(image); UnloadRenderTexture(rt);
        }
    }
    void uniform(Shader shader,const char* name,float value) const {
        int loc=GetShaderLocation(shader,name); if (loc>=0) SetShaderValue(shader,loc,&value,SHADER_UNIFORM_FLOAT);
    }
    void surfaces(float mode) const {
        rlDrawRenderBatchActive(); uniform(world,"surfaceMode",mode); uniform(world,"normalMapped",mode==1?1.f:0.f);
    }
    void worldUniforms(const nf::Game& g,Camera3D camera) const {
        auto forward=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
        SetShaderValue(world,GetShaderLocation(world,"viewPos"),&camera.position,SHADER_UNIFORM_VEC3);
        SetShaderValue(world,GetShaderLocation(world,"flashDir"),&forward,SHADER_UNIFORM_VEC3);
        uniform(world,"flashOn",g.flashlight && g.battery>0 ? 1.0f:0.0f);
        uniform(world,"time",clock);
        uniform(world,"tension",1-g.sanity/100);
        uniform(world,"shotFlash",saturate((g.shotTimer-.35f)/.08f));
    }
    void signQuad(Texture2D texture,Vector3 p,bool side,int face,float width=2.05f,float height=.43f) const {
        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);
        rlColor4ub(225,225,212,255);
        rlNormal3f(side?static_cast<float>(face):0,0,side?0:1);
        auto vertex=[&](float u,float vv,float a,float b) {
            rlTexCoord2f(u,vv);
            rlVertex3f(p.x+(side?0:a),p.y+b,p.z+(side?-a*face:0));
        };
        // Double sided to keep signage readable from both corridor approaches.
        vertex(0,1,-width/2,-height/2); vertex(1,1,width/2,-height/2);
        vertex(1,0,width/2,height/2); vertex(0,0,-width/2,height/2);
        vertex(0,0,-width/2,height/2); vertex(1,0,width/2,height/2);
        vertex(1,1,width/2,-height/2); vertex(0,1,-width/2,-height/2);
        rlEnd(); rlSetTexture(0);
    }
    void station(const nf::Game& g) const {
        Color metal{43,56,58,255};
        surfaces(1);
        DrawModelEx(models.floorCube,{0,-.18f,-4.5f},{0,1,0},0,{20.3f,.35f,37.3f},Color{192,201,192,255});
        DrawModelEx(models.wallCube,{0,3.88f,-4.5f},{0,1,0},0,{20.3f,.18f,37.3f},Color{131,149,142,255});
        for (const auto& b:g.walls) {
            DrawModelEx(models.wallCube,v(b.center),{0,1,0},0,v(b.size),Color{191,202,191,255});
        }
        surfaces(0);
        for (const auto& b:g.walls) {
            if (b.size.y>2) {
                cube({b.center.x,.21f,b.center.z},{b.size.x+.035f,.21f,b.size.z+.035f},metal);
                cube({b.center.x,1.04f,b.center.z},{b.size.x+.04f,.045f,b.size.z+.04f},Color{93,58,48,255});
            }
        }
        for (int i=0;i<19;++i) {
            float z=13-i*2.0f;
            cube({0,.004f,z},{3.95f,.012f,.018f},Color{32,48,48,255});
            for (float x:{-1.55f,1.55f}) cube({x,.012f,z},{.045f,.012f,1.6f},Color{136,119,70,255});
        }
        for (int i=0;i<7;++i) {
            float z=12-i*5.4f;
            for (float x:{-1.92f,1.92f}) {
                cube({x,2.0f,z},{.13f,3.65f,.23f},metal);
                cube({x,3.65f,z},{.3f,.24f,.38f},metal);
            }
            cube({0,3.65f,z},{4,.14f,.24f},metal);
            cube({0,3.72f,z-1.4f},{1.45f,.09f,.32f},Color{24,35,37,255});
        }
        // Exposed pipes run the full length of the station.
        for (float x:{-1.61f,-1.40f,1.64f}) {
            DrawCylinderEx({x,3.38f,13},{x,3.38f,-22.8f},.055f,.055f,8,metal);
            for (int i=0;i<9;++i) cube({x,3.38f,12-i*4.1f},{.16f,.19f,.07f},Color{93,79,61,255});
        }
        for (std::size_t i=0;i<g.rooms.size();++i) {
            auto p=v(g.rooms[i].center); float side=p.x<0?-1.f:1.f;
            for (float dz:{-1.27f,1.27f}) cube({side*1.85f,1.45f,p.z+dz},{.17f,2.9f,.13f},metal);
            cube({side*1.85f,2.96f,p.z},{.17f,.14f,2.68f},metal);
            surfaces(2);
            VisualAssets::draw(models.utility,{p.x+side*2.75f,0,p.z-2.25f},1.9f,side<0?90.f:-90.f);
            VisualAssets::draw(models.utility,{p.x+side*2.75f,0,p.z+1.9f},1.6f,side<0?90.f:-90.f);
            VisualAssets::draw(models.chair,{p.x-.6f,0,p.z-1.8f},1.05f,i%2?23.f:156.f);
            VisualAssets::draw(models.barrel,{p.x+1.3f,0,p.z+3.7f},.91f,-25.f);
            surfaces(0);
            for(int j=0;j<9;++j)
                cube({p.x-2.6f+j*.6f,2.3f,p.z-4.88f},{.36f,1.7f,.12f},Color{48,62,60,255});
        }
        // Heavy elevator doors and a transmitter wall form the two corridor endpoints.
        cube({0,1.6f,13.77f},{2.8f,3.2f,.12f},Color{30,41,44,255});
        for(float x:{-.71f,.71f}) cube({x,1.6f,13.67f},{1.35f,3.1f,.06f},metal);
        cube({0,1.6f,13.62f},{.035f,3.1f,.04f},ink);
        for(float x:{-1.5f,1.5f}) cube({x,1.65f,13.56f},{.13f,3.3f,.15f},Color{139,112,59,255});
        for(float x:{-1.25f,0.f,1.25f}) {
            cube({x,1.36f,-22.65f},{.9f,2.55f,.46f},Color{24,39,39,255});
            for(int j=0;j<13;++j) cube({x, .36f+j*.15f,-22.38f},{.7f,.05f,.025f},Color{66,83,77,255});
        }
        // Long cables and shallow puddles catch the light.
        for(int i=0;i<5;++i) {
            float z=4-i*5.2f;
            DrawCylinderEx({-1.86f,3.35f,z},{-1.75f,2.2f,z+.3f},.016f,.016f,5,Color{12,23,24,255});
            cube({.65f*std::sin(i*3.f),.021f,z},{.7f+i*.08f,.005f,.85f},Color{41,67,68,255});
        }
    }
    void pickups(const nf::Game& g) const {
        for(const auto& p:g.pickups) {
            if(p.collected) continue;
            Vector3 at=v(p.position);
            if(p.type==nf::PickupType::Tape) {
                surfaces(2); VisualAssets::draw(models.barrel,{at.x,0,at.z},at.y-.05f,35); surfaces(0);
                rlPushMatrix(); rlTranslatef(at.x,at.y+.06f+std::sin(clock*2)*.015f,at.z); rlRotatef(12,0,1,0);
                cube({0,0,0},{.44f,.14f,.29f},Color{169,155,112,255});
                cube({0,.075f,.01f},{.34f,.008f,.14f},Color{32,40,36,255});
                for(float x:{-.1f,.1f}) DrawCylinderEx({x,.078f,0},{x,.085f,0},.055f,.055f,16,Color{145,159,140,255});
                cube({0,.086f,-.105f},{.3f,.01f,.035f},signal);
                rlPopMatrix();
            } else {
                Color c=p.type==nf::PickupType::Medkit ? Color{159,164,141,255} : (p.type==nf::PickupType::Ammo?Color{123,101,64,255}:Color{69,111,112,255});
                cube(at,{.36f,.22f,.25f},c);
                if(p.type==nf::PickupType::Medkit) {
                    cube({at.x,at.y+.12f,at.z},{.08f,.012f,.2f},signal);
                    cube({at.x,at.y+.12f,at.z},{.23f,.012f,.06f},signal);
                }
            }
        }
    }
    void luminous(const nf::Game& g) const {
        for(int i=0;i<7;++i) {
            float z=10.6f-i*5.4f;
            float flicker=(i%3==0 && std::sin(clock*9+i)*std::sin(clock*17)<-.5f)?.18f:1.f;
            cube({0,3.665f,z},{1.3f,.025f,.18f},Fade(i%3==1?Color{202,155,95,255}:Color{161,216,199,255},flicker));
        }
        for(std::size_t i=0;i<g.rooms.size();++i) {
            auto p=v(g.rooms[i].center); float side=p.x<0?-1.f:1.f;
            signQuad(signs[i],{side*1.748f,3.22f,p.z},true,side<0?1:-1);
            cube({side*1.74f,2.73f,p.z-1.38f},{.035f,.08f,.08f},i%2?signal:mint);
        }
        signQuad(signs[6],{0,3.18f,-22.8f},false,1,2.7f,.56f);
        cube({1.7f,1.35f,13.5f},{.12f,.18f,.12f},g.tapes==3?mint:signal);
        for(const auto& p:g.pickups) if(!p.collected) {
            auto c=p.type==nf::PickupType::Tape?mint:Color{173,152,102,255};
            DrawSphere({p.position.x,p.position.y+.2f,p.position.z},.026f,c);
        }
    }
    Camera3D camera(const nf::Game& g,bool menu) const {
        Vector3 pos=v(g.player), f=v(g.forward());
        if(menu) { pos={.65f,1.65f,10.8f}; f={-.045f,.025f,-1}; }
        else { pos.y+=reduced?0:std::sin(walkPhase*2)*.014f; }
        float roll=reduced?0:std::sin(clock*.9f)*(1-g.sanity/100)*.013f;
        return {pos,Vector3Add(pos,f),{roll,1,0},mix(73,54,aim),CAMERA_PERSPECTIVE};
    }
    void gun(const nf::Game& g) { weaponView.draw(g,reduced); }
    void render(const nf::Game& g,bool menu) {
        resize();
        auto cam=camera(g,menu);
        auto direction=Vector3Normalize(Vector3Subtract(cam.target,cam.position));
        auto right=Vector3Normalize(Vector3CrossProduct(direction,cam.up));
        Camera3D light=cam;
        light.position=Vector3Add(cam.position,Vector3Add(Vector3Scale(right,.22f),Vector3{0,.08f,0}));
        light.target=Vector3Add(cam.position,Vector3Scale(direction,20)); light.fovy=80;
        shadows.unbind();
        uniform(world,"depthPass",1); uniform(world,"shadowsEnabled",0);
        double nearPlane=rlGetCullDistanceNear(),farPlane=rlGetCullDistanceFar();
        rlSetClipPlanes(.1,45);
        BeginTextureMode(shadows.target); ClearBackground(WHITE); BeginMode3D(light);
        Matrix lightVP=MatrixMultiply(rlGetMatrixModelview(),rlGetMatrixProjection());
        BeginShaderMode(world); station(g); pickups(g); surfaces(2);
        for(std::size_t i=0;i<g.enemies.size();++i) models.figure(i,g.enemies[i],g.player,clock);
        EndShaderMode(); EndMode3D(); EndTextureMode();
        rlSetClipPlanes(nearPlane,farPlane);
        uniform(world,"depthPass",0); uniform(world,"shadowsEnabled",1);
        SetShaderValueMatrix(world,GetShaderLocation(world,"lightVP"),lightVP);
        SetShaderValue(world,GetShaderLocation(world,"flashPos"),&light.position,SHADER_UNIFORM_VEC3);
        if(!menu) {
            // Isolated depth keeps the revolver visible near walls and preserves self occlusion.
            BeginTextureMode(weaponLayer); ClearBackground(BLANK); gun(g); EndTextureMode();
        }
        BeginTextureMode(scene);
        ClearBackground(Color{17,29,32,255});
        worldUniforms(g,cam);
        BeginMode3D(cam); BeginShaderMode(world);
        shadows.bind(world);
        station(g); pickups(g);
        surfaces(2);
        for(std::size_t i=0;i<g.enemies.size();++i) models.figure(i,g.enemies[i],g.player,clock);
        surfaces(0);
        EndShaderMode(); luminous(g);
        // Very fine suspended dust gives the flashlight a sense of volume.
        for(int i=0;i<85;++i) {
            float z=12-std::fmod(i*1.731f+clock*.05f,35.0f);
            DrawSphere({std::sin(i*17.1f)*1.75f, .45f+std::fmod(i*.37f+clock*.024f,2.9f),z},.005f,Fade(muted,.28f));
        }
        EndMode3D();
        if(!menu) DrawTexturePro(weaponLayer.texture,{0,0,static_cast<float>(sceneWidth),-static_cast<float>(sceneHeight)},
            {0,0,static_cast<float>(sceneWidth),static_cast<float>(sceneHeight)},{0,0},0,WHITE);
        EndTextureMode();
        BeginDrawing(); ClearBackground(ink);
        uniform(post,"time",clock); uniform(post,"tension",reduced?0.f:1-g.sanity/100);
        uniform(post,"damage",g.damageFlash); uniform(post,"glitched",reduced?0.f:g.horrorPulse);
        uniform(post,"grain",reduced?0.f:1.f);
        Vector2 resolution{static_cast<float>(sceneWidth),static_cast<float>(sceneHeight)};
        SetShaderValue(post,GetShaderLocation(post,"resolution"),&resolution,SHADER_UNIFORM_VEC2);
        BeginShaderMode(post);
        DrawTexturePro(scene.texture,{0,0,static_cast<float>(sceneWidth),-static_cast<float>(sceneHeight)},
                       {0,0,static_cast<float>(GetScreenWidth()),static_cast<float>(GetScreenHeight())},{0,0},0,WHITE);
        EndShaderMode();
    }
    void bar(float x,float y,float w,float value,Color color) const {
        DrawRectangleRec({x,y,w,3},Fade(muted,.15f));
        DrawRectangleRec({x,y,w*saturate(value/100),3},color);
    }
    void hud(const nf::Game& g,bool map) const {
        beginUI();
        text("СТАНЦИЯ 00",48,37,21,paper,3);
        text("ПРОТОКОЛ ВОЗВРАЩЕНИЯ",48,69,12,muted,2);
        text("ЗАПИСИ",1168,38,15,muted,2);
        for(int i=0;i<3;++i) {
            DrawRectangleRec({1273.f+i*37,39,24,16},i<g.tapes?mint:Fade(muted,.12f));
            DrawRectangleLinesEx({1273.f+i*37,39,24,16},1,i<g.tapes?mint:muted);
        }
        text(g.tapes==3?"ВЕРНИТЕСЬ К ЛИФТУ":"НАЙДИТЕ ТРИ ЗАПИСИ",1168,70,12,g.tapes==3?mint:muted,1);
        text("СОСТОЯНИЕ",48,736,12,muted,2);
        heading(TextFormat("%03i",static_cast<int>(std::max(0.f,g.health))),46,753,48,paper,1);
        bar(49,816,160,g.health,g.health<30?signal:paper);
        text("РАССУДОК",246,753,12,muted,1.2f); bar(246,781,124,g.sanity,mint);
        text("ВЫНОСЛИВОСТЬ",246,800,11,muted,1); bar(246,826,124,g.stamina,paper);
        text(g.flashlight?"ФОНАРЬ / ВКЛ":"ФОНАРЬ / ВЫКЛ",423,800,11,g.flashlight?muted:signal,1);
        bar(423,826,110,g.battery,g.battery<20?signal:mint);
        heading(TextFormat("%02i",g.ammo),1207,733,76,paper,1);
        heading(TextFormat("/ %02i",g.reserve),1301,773,34,muted,1);
        text(g.reloadTimer>0?"ПЕРЕЗАРЯДКА...":"РЕВОЛЬВЕР / .38",1208,826,12,g.reloadTimer>0?signal:muted,1.5f);
        if(g.reloadTimer>0) bar(1208,856,165,(1-g.reloadTimer/1.85f)*100,signal);
        if(!map) {
            Color c=g.hitThisFrame?signal:Fade(paper,.72f); float gap=5+recoil*11;
            DrawLineEx({720-gap-5,450},{720-gap,450},1,c); DrawLineEx({720+gap,450},{720+gap+5,450},1,c);
            DrawLineEx({720,450-gap-5},{720,450-gap},1,c); DrawLineEx({720,450+gap},{720,450+gap+5},1,c);
            DrawCircle(720,450,1,c);
            if(const auto* p=g.nearbyPickup()) {
                const char* label=p->type==nf::PickupType::Tape?"ЗАБРАТЬ ЗАПИСЬ":p->type==nf::PickupType::Ammo?"ЗАБРАТЬ ПАТРОНЫ":p->type==nf::PickupType::Medkit?"ВЗЯТЬ АПТЕЧКУ":"ВЗЯТЬ БАТАРЕЮ";
                DrawRectangleRec({540,526,360,44},Fade(ink,.86f)); text("E",558,535,19,mint); text(label,595,537,15);
            } else if(Vector3Distance(v(g.player),Vector3{g.exitPosition.x,g.player.y,g.exitPosition.z})<2.1f) {
                centered(g.tapes==3?"E   —   ПОКИНУТЬ СТАНЦИЮ":"ЛИФТ ЗАБЛОКИРОВАН / НУЖНЫ ТРИ ЗАПИСИ",530,16,g.tapes==3?mint:muted);
            }
            if(g.messageTimer>0 && !g.message.empty()) {
                float width=std::min(1050.f,MeasureTextEx(ui,g.message.c_str(),20,.7f).x+48);
                DrawRectangleRec({720-width/2,676,width,49},Fade(ink,.83f)); centered(g.message,688,20,paper);
            }
        }
        centered("TAB  карта     F  фонарь     R  перезарядка     I  осмотр     ESC  пауза",863,12,Fade(muted,.8f));
        if(map) mapOverlay(g);
        EndMode2D();
    }
    void mapOverlay(const nf::Game& g) const {
        DrawRectangleRec({300,95,840,714},Color{9,20,23,246});
        DrawRectangleLinesEx({300,95,840,714},1,Fade(mint,.3f));
        text("ПЛАН СТАНЦИИ",335,119,23,paper,3);
        text("БИРЮЗОВЫЕ МЕТКИ — ЗАПИСИ",335,159,12,mint,1.3f);
        auto point=[](nf::Vec3 p)->Vector2 { return {720+p.x*20,230+(p.z+23)*13.2f}; };
        for(auto b:g.walls) {
            auto p=point(b.center);
            DrawRectangleRec({p.x-b.size.x*10,p.y-b.size.z*6.6f,std::max(2.f,b.size.x*20),std::max(2.f,b.size.z*13.2f)},Color{67,92,89,255});
        }
        for(const auto& p:g.pickups) if(!p.collected && p.type==nf::PickupType::Tape) {
            auto at=point(p.position); DrawCircleV(at,5,mint); DrawCircleLines(static_cast<int>(at.x),static_cast<int>(at.y),11,Fade(mint,.4f));
        }
        auto exit=point(g.exitPosition); DrawRectangleRec({exit.x-10,exit.y-8,20,16},g.tapes==3?mint:signal);
        auto p=point(g.player); DrawCircleV(p,5,paper);
        DrawLineEx(p,{p.x+std::sin(g.yaw)*17,p.y-std::cos(g.yaw)*17},2,paper);
        text("ВЫХОД / ЛИФТ",767,705,12,g.tapes==3?mint:signal);
        text("Удерживайте TAB. Следите за тем, что остаётся за спиной.",335,763,14,muted);
    }
    void controls() const {
        DrawRectangleRec({330,118,780,660},Color{10,21,24,250});
        DrawRectangleLinesEx({330,118,780,660},1,Fade(muted,.35f));
        heading("УПРАВЛЕНИЕ",374,143,46,paper,2);
        const char* keys[]={"W A S D","МЫШЬ","ЛКМ / ПКМ","R / E","SHIFT","F / TAB","I","F11 / M / V","ESC"};
        const char* labels[]={"Движение","Осмотреться","Выстрел / прицелиться","Перезарядить / взаимодействовать","Бежать","Фонарь / карта станции","Осмотреть оружие","Полный экран / звук / мягкие эффекты","Пауза / вернуться"};
        for(int i=0;i<9;++i) { text(keys[i],377,220+i*43,17,mint,1); text(labels[i],590,220+i*43,17,paper,.1f); }
        text("Найдите три кассеты. Вернитесь к лифту и нажмите E.",377,635,17,paper);
        text("Фонарь восстанавливается, пока выключен.",377,668,16,muted);
        text("Некоторые силуэты исчезают, когда вы отводите взгляд.",377,700,16,muted);
        centered("Нажмите ESC или ENTER, чтобы вернуться",744,14,muted);
    }
    void menu(bool paused,bool help,const nf::Game& g,int& action) const {
        DrawRectangleGradientH(0,0,GetScreenWidth(),GetScreenHeight(),Color{5,13,17,249},Color{5,13,17,40});
        beginUI();
        DrawRectangle(65,71,30,3,signal); text("СТАНЦИЯ 00 / СЕАНС 01",109,59,14,muted,3);
        if(paused) {
            heading("СИГНАЛ ПРИОСТАНОВЛЕН",65,150,67,paper,1);
            text("Они всё ещё ждут по ту сторону.",69,246,20,muted);
        } else {
            heading("NULL",61,139,130,paper,7);
            heading("FREQUENCY",61,285,130,paper,5);
            text("Н У Л Е В А Я   Ч А С Т О Т А",70,463,20,muted,1);
            DrawRectangle(70,515,48,2,signal);
            text("Вы услышали собственный голос в эфире.",70,543,19,paper);
            text("Передача началась до вашего рождения.",70,574,19,muted);
        }
        float y=paused?340.f:642.f;
        if(button(paused?"ПРОДОЛЖИТЬ":"НАЧАТЬ СЕАНС",{70,y,370,58},true)) action=1;
        if(button(paused?"НАЧАТЬ ЗАНОВО":"УПРАВЛЕНИЕ",{70,y+73,370,52})) action=paused?2:3;
        if(button("ВЫХОД",{70,y+140,370,52})) action=4;
        text(paused?"ESC / продолжить":"ENTER / войти в эфир",70,y-29,12,muted,1.8f);
        text("ПСИХОЛОГИЧЕСКИЙ ХОРРОР / ШУТЕР ОТ ПЕРВОГО ЛИЦА",70,861,11,muted,1.8f);
        text("v0.1.0",1301,861,12,muted,1);
        // A small receiver display establishes the visual language of the station.
        DrawRectangleRec({1120,632,251,162},Color{9,23,26,214});
        DrawRectangleLinesEx({1120,632,251,162},1,Fade(muted,.22f));
        text("LIVE / 000.0 MHz",1140,651,12,mint,2);
        for(int i=0;i<43;++i) {
            float height=4+std::abs(std::sin(i*.47f+clock*1.5f)*std::sin(i*.9f-clock))*39;
            DrawRectangleRec({1140+i*5.f,723-height*.5f,2,height},Fade(mint,.7f));
        }
        text("НЕ ОТВЕЧАЙТЕ ГОЛОСУ",1140,764,11,muted,1);
        if(help) controls();
        EndMode2D();
        (void)g;
    }
    void ending(const nf::Game& g,int& action) const {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Color{5,12,16,227});
        beginUI();
        centered(g.won?"ПЕРЕДАЧА ПРЕРВАНА":"СИГНАЛ ПОТЕРЯН",230,17,g.won?mint:signal);
        const char* line=g.won?"ВЫ ВЫШЛИ В ЭФИР.":"ОНИ УСЛЫШАЛИ ВАС.";
        float w=MeasureTextEx(title,line,76,2).x; heading(line,(1440-w)/2,290,76,paper,2);
        centered(g.won?"Лифт поднимается. Ваш голос остаётся внизу.":"На станции появилась ещё одна запись.",418,21,muted);
        centered(TextFormat("ЗАПИСИ %i / 3     УСТРАНЕНО %i     ВРЕМЯ %02i:%02i",g.tapes,g.kills,static_cast<int>(g.time)/60,static_cast<int>(g.time)%60),492,16,paper);
        if(button("НОВЫЙ СЕАНС",{535,587,370,58},true)) action=2;
        if(button("В ГЛАВНОЕ МЕНЮ",{535,662,370,52})) action=5;
        EndMode2D();
    }
    void unload() {
        models.unload();
        shadows.unload();
        weaponView.unload();
        for(auto t:signs) UnloadTexture(t);
        if(scene.id) UnloadRenderTexture(scene);
        if(weaponLayer.id) UnloadRenderTexture(weaponLayer);
        UnloadShader(world); UnloadShader(post); UnloadShader(weaponShader); UnloadFont(ui); UnloadFont(title);
    }
};

bool findAssets(const char* argv0) {
    namespace fs=std::filesystem;
    std::vector<fs::path> roots{fs::current_path()};
    std::error_code ec;
    auto executable=fs::absolute(argv0,ec);
#ifdef __linux__
    auto actual=fs::read_symlink("/proc/self/exe",ec); if(!ec) executable=actual;
#endif
    if(!executable.empty()) { roots.push_back(executable.parent_path()); roots.push_back(executable.parent_path().parent_path()); }
#ifdef NF_SOURCE_DIR
    roots.emplace_back(NF_SOURCE_DIR);
#endif
    for(const auto& root:roots) if(fs::exists(root/"assets/shaders/world.fs")) { fs::current_path(root); return true; }
    std::cerr << "Cannot find assets/. Run beside the assets directory.\n";
    return false;
}
} // namespace

int main(int argc,char** argv) {
    bool smoke=false,autoplay=false,showcase=false;
    for(int i=1;i<argc;++i) {
        std::string arg=argv[i];
        if(arg=="--smoke-test") smoke=true;
        else if(arg=="--animation-preview") { smoke=true; autoplay=true; showcase=true; }
        else if(arg=="--play") autoplay=true;
        else if(arg=="--help") { std::cout<<"NULL FREQUENCY 0.1.0\n--play: skip title screen\n--smoke-test: render 100 hidden frames and export screenshots to docs/\n--animation-preview: render FPS animation frames to build/animation-frames/\n"; return 0; }
    }
    if(!findAssets(argv[0])) return 1;
    SetTraceLogLevel(LOG_WARNING);
    unsigned int flags=FLAG_WINDOW_RESIZABLE|FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT;
    if(smoke) flags|=FLAG_WINDOW_HIDDEN;
    SetConfigFlags(flags);
    InitWindow(showcase?960:1440,showcase?600:900,"NULL FREQUENCY | Нулевая частота");
    if(!IsWindowReady()) return 2;
    SetWindowMinSize(960,600);
    SetExitKey(KEY_NULL);
    SetTargetFPS(smoke?240:120);
    Presentation art;
    if(!art.load()) { std::cerr<<"Graphics/font/shader initialization failed.\n"; CloseWindow(); return 3; }
    nf::Audio audio; bool audioReady=audio.init();
    if(smoke&&audioReady) SetMasterVolume(0);
    if(showcase) std::filesystem::create_directories("build/animation-frames");
    nf::Game game;
    bool playing=autoplay,paused=false,help=false,exit=false;
    if(playing&&!smoke) DisableCursor();
    bool triggerArmed=false;
    int frames=0,lastCollected=0; float lastHealth=game.health,whisperCooldown=0;
    while(!exit&&!WindowShouldClose()) {
        float dt=smoke?1.f/60:std::min(GetFrameTime(),.05f);
        art.clock+=dt; whisperCooldown=std::max(0.f,whisperCooldown-dt);
        if(IsKeyPressed(KEY_F11)) {
            if(IsWindowFullscreen()) { ToggleFullscreen(); SetWindowSize(1440,900); }
            else { int monitor=GetCurrentMonitor(); SetWindowSize(GetMonitorWidth(monitor),GetMonitorHeight(monitor)); ToggleFullscreen(); }
        }
        if(IsKeyPressed(KEY_M)) { art.mutedAudio=!art.mutedAudio; if(audioReady) SetMasterVolume(art.mutedAudio?0.f:.55f); }
        if(IsKeyPressed(KEY_V)) art.reduced=!art.reduced;
        if(IsKeyPressed(KEY_ESCAPE)) {
            if(help) help=false;
            else if(playing&&!game.dead&&!game.won) { paused=!paused; if(paused) EnableCursor(); else DisableCursor(); }
            else if(playing) { playing=false; paused=false; EnableCursor(); }
        }
        if(!IsWindowFocused()&&playing&&!smoke&&!paused) { paused=true; EnableCursor(); }
        if(smoke&&!showcase&&frames==60) { game.player={3.15f,1.65f,7.8f}; game.yaw=PI_F*.5f; game.pitch=-.035f; }
        if(smoke&&!showcase&&frames==90) { paused=true; help=true; }
        if(smoke&&!showcase&&frames==96) { paused=false; game.dead=true; game.health=0; }
        bool active=playing&&!paused&&!game.dead&&!game.won;
        bool map=active&&(IsKeyDown(KEY_TAB)||(smoke&&!showcase&&frames>=85&&frames<90));
        if(smoke&&!showcase&&frames==18) { playing=true; active=true; }
        if(active) {
            Vector2 weaponMouse{};
            if(!smoke) {
                auto mouse=GetMouseDelta();
                weaponMouse=mouse;
                game.yaw+=mouse.x*.00205f*(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)?.65f:1.f);
                game.pitch=std::clamp(game.pitch-mouse.y*.0019f,-1.35f,1.35f);
            }
            nf::Input input;
            input.forward=static_cast<float>(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))-static_cast<float>(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN));
            input.strafe=static_cast<float>(IsKeyDown(KEY_D))-static_cast<float>(IsKeyDown(KEY_A));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) triggerArmed=true;
            input.sprint=IsKeyDown(KEY_LEFT_SHIFT); input.fire=IsMouseButtonDown(MOUSE_BUTTON_LEFT)&&triggerArmed&&!map;
            input.reload=IsKeyDown(KEY_R); input.interact=IsKeyDown(KEY_E); input.toggleFlashlight=IsKeyDown(KEY_F);
            if(smoke) {
                input={};
                if(showcase) {
                    if(frames>=30&&frames<80) input.forward=1;
                    input.sprint=frames>=65&&frames<80;
                    if(frames==85) input.fire=true;
                    if(frames==95) { game.ammo=0; input.reload=true; }
                    weaponMouse={std::sin(frames*.06f)*3, std::cos(frames*.05f)*1.5f};
                } else { if(frames==75) input.fire=true; if(frames==82) input.reload=true; }
            }
            float previousReload=game.reloadTimer;
            game.update(dt,input);
            bool moving=std::abs(input.forward)+std::abs(input.strafe)>.1f;
            if(moving) art.walkPhase+=dt*(input.sprint?13.f:8.f);
            bool aiming=IsMouseButtonDown(MOUSE_BUTTON_RIGHT)||(showcase&&frames>=220&&frames<250);
            bool inspecting=IsKeyDown(KEY_I)||(showcase&&frames==270);
            art.aim=mix(art.aim,aiming&&!input.sprint&&game.reloadTimer<=0?1.f:0.f,1-std::exp(-dt*12));
            art.weaponView.update(dt,game,weaponMouse,moving,input.sprint,aiming,inspecting);
            if(game.firedThisFrame) { art.recoil=1; if(audioReady) audio.shoot(); }
            if(previousReload<=0&&game.reloadTimer>0&&audioReady) audio.reload();
            int collected=static_cast<int>(std::count_if(game.pickups.begin(),game.pickups.end(),[](const nf::Pickup& p){return p.collected;}));
            if(collected!=lastCollected&&audioReady) audio.pickup();
            if(game.health<lastHealth&&audioReady) audio.hurt();
            if(game.horrorPulse>.60f&&whisperCooldown<=0&&audioReady) { audio.whisper(); whisperCooldown=9; }
            lastHealth=game.health; lastCollected=collected;
            if(audioReady) {
                float proximity=0,pan=.5f;
                for(const auto& e:game.enemies) if(e.alive&&e.kind==0) {
                    float dx=e.position.x-game.player.x,dz=e.position.z-game.player.z;
                    float distance=std::max(.01f,std::hypot(dx,dz));
                    float strength=saturate(1-distance/12);
                    nf::Vec3 toward{dx,0,dz};
                    if(game.wallDistance(game.player,toward,distance)<distance-.2f) strength*=.3f;
                    if(strength>proximity) { proximity=strength; pan=.5f+.5f*(dx*std::cos(game.yaw)+dz*std::sin(game.yaw))/distance; }
                }
                audio.threat(pan,proximity);
                audio.update(dt,1-game.sanity/100,moving,input.sprint,game.flashlight);
            }
            if(game.won||game.dead) EnableCursor();
        } else if(audioReady) { audio.threat(.5f,0); audio.update(dt,.1f,false,false,false); }
        art.recoil=mix(art.recoil,0,1-std::exp(-dt*15));
        art.render(game,!playing);
        int action=0;
        if(!playing||paused) {
            art.menu(paused,help,game,action);
            if(IsKeyPressed(KEY_ENTER)) { if(help) help=false; else action=1; }
        } else if(game.won||game.dead) art.ending(game,action);
        else art.hud(game,map);
        // Export the actual composed frame while the graphics context is alive.
        if(smoke&&!showcase&&(frames==12||frames==58||frames==80)) {
            std::filesystem::create_directories("docs");
            Image capture=LoadImageFromScreen();
            const char* path=frames==12?"docs/menu.png":frames==58?"docs/screenshot.png":"docs/encounter.png";
            if(!ExportImage(capture,path)) {
                UnloadImage(capture); std::cerr<<"Screenshot export failed.\n"; return 4;
            }
            UnloadImage(capture);
        }
        if(showcase&&frames%2==0) {
            Image capture=LoadImageFromScreen();
            std::string path=TextFormat("build/animation-frames/frame-%04i.png",frames/2);
            ExportImage(capture,path.c_str()); UnloadImage(capture);
        }
        EndDrawing();
        if(action==1) { playing=true; paused=false; help=false; triggerArmed=false; if(!smoke) DisableCursor(); }
        if(action==2) { game.reset(); playing=true; paused=false; help=false; lastCollected=0; lastHealth=100; triggerArmed=false; if(!smoke) DisableCursor(); }
        if(action==3) help=!help;
        if(action==4) exit=true;
        if(action==5) { game.reset(); playing=false; paused=false; EnableCursor(); }
        if(smoke&&++frames>=(showcase?420:100)) break;
    }
    audio.shutdown(); art.unload(); CloseWindow();
    if(smoke) std::cout<<"SMOKE_OK: "<<frames<<" frames, fonts, world/post shaders, menu, FPS HUD, hands/weapon rig, audio="<<(audioReady?"ready":"unavailable")<<"\n";
    return 0;
}
