#pragma once

#include "core.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace nf {

// An isolated, animated first-person rig. draw() owns its Camera3D and is called
// inside the caller's transparent weapon RenderTexture, outside BeginMode3D.
class WeaponView {
public:
    bool load(Shader shader) {
        shader_=shader;
        gun_=LoadModel("assets/models/weapon/weapon.glb");
        hands_=LoadModel("assets/models/hands/hands.glb");
        if(!IsModelValid(gun_) || !IsModelValid(hands_) || hands_.boneCount<20) return false;
        prepare(gun_,true); prepare(hands_,false);
        splitMechanism();
        restLocal_.resize(hands_.boneCount); posed_.resize(hands_.boneCount);
        for(int i=0;i<hands_.boneCount;++i) {
            restLocal_[i]=hands_.bindPose[i];
            const int parent=hands_.bones[i].parent;
            if(parent>=0) {
                const auto& p=hands_.bindPose[parent];
                restLocal_[i].translation=Vector3Divide(Vector3RotateByQuaternion(
                    Vector3Subtract(hands_.bindPose[i].translation,p.translation),QuaternionInvert(p.rotation)),p.scale);
                restLocal_[i].rotation=QuaternionMultiply(QuaternionInvert(p.rotation),hands_.bindPose[i].rotation);
                restLocal_[i].scale=Vector3Divide(hands_.bindPose[i].scale,p.scale);
            }
        }
        paraRig_=bone("hand.R")>=0;
        handScale_=paraRig_?1.0f:.083f;
        rightWrist_=bone(paraRig_?"hand.R":"wrist.r");
        leftWrist_=bone(paraRig_?"hand.L":"wrist.l");
        ready_=rightWrist_>=0 && leftWrist_>=0;
        return ready_;
    }

    void update(float dt,const Game& game,Vector2 mouseDelta,bool moving,
                bool sprinting,bool aiming,bool inspect) {
        dt=std::clamp(dt,0.0f,.1f); clock_+=dt;
        const bool reloading=game.reloadTimer>0;
        if(reloading && !wasReloading_) {
            reloadRounds_=std::min(6-game.ammo,game.reserve);
            spent_=6-game.ammo; inspectTime_=-1;
        }
        reloadProgress_=reloading ? std::clamp(1-game.reloadTimer/1.85f,0.0f,1.0f) : -1;
        wasReloading_=reloading;
        if(inspect && !inspectHeld_ && !reloading && !sprinting && !aiming) inspectTime_=0;
        inspectHeld_=inspect;
        if(inspectTime_>=0) { inspectTime_+=dt; if(inspectTime_>2.45f) inspectTime_=-1; }
        if(reloading || aiming || sprinting) inspectTime_=-1;
        const bool canAim=aiming && !reloading && !sprinting && inspectTime_<0;
        ads_=approach(ads_,canAim?1.0f:0.0f,dt,14);
        movement_=approach(movement_,moving?1.0f:0.0f,dt,9);
        sprint_=approach(sprint_,sprinting&&moving?1.0f:0.0f,dt,9);
        equip_=approach(equip_,1,dt,5);
        bobPhase_+=dt*(sprinting?12.5f:8.3f)*(moving?1.0f:.08f);
        const float damp=1-std::exp(-dt*11);
        sway_.x+=(std::clamp(-mouseDelta.x*.0009f,-.023f,.023f)-sway_.x)*damp;
        sway_.y+=(std::clamp(mouseDelta.y*.0007f,-.019f,.019f)-sway_.y)*damp;
        swayAngle_.x+=(std::clamp(mouseDelta.y*.18f,-4.0f,4.0f)-swayAngle_.x)*damp;
        swayAngle_.y+=(std::clamp(mouseDelta.x*.16f,-4.0f,4.0f)-swayAngle_.y)*damp;
        if(game.firedThisFrame) { kick_=std::min(1.5f,kick_+1); cylinderTarget_-=60; }
        kick_=std::max(0.0f,kick_-dt*6.8f);
        cylinderAngle_=approach(cylinderAngle_,cylinderTarget_,dt,24);
        poseHands();
    }

    void draw(const Game& game,bool reduced) {
        if(!ready_) return;
        const float motion=reduced?.23f:1.0f;
        const float p=reloadProgress_;
        const float reload=p>=0 ? smooth(.0f,.15f,p)*(1-smooth(.82f,1,p)) : 0;
        const float open=p>=0 ? smooth(.12f,.29f,p)*(1-smooth(.80f,.94f,p)) : 0;
        const float eject=p>=0 ? smooth(.28f,.45f,p)*(1-smooth(.49f,.58f,p)) : 0;
        const float inspection=inspectTime_>=0 ? std::sin(std::clamp(inspectTime_/2.45f,0.0f,1.0f)*PI) : 0;
        const float settle=(1-equip_)*(1-equip_);
        const float walk=movement_*(1-ads_*.85f)*motion;
        Vector3 position{mix(.195f,0.0f,ads_),mix(-.185f,-.1375f,ads_),mix(-.49f,-.475f,ads_)};
        position.x+=sway_.x*(1-ads_*.80f)*motion+std::sin(bobPhase_)*.010f*walk;
        position.y+=sway_.y*(1-ads_*.80f)*motion+std::cos(bobPhase_*2)*.006f*walk;
        position.y+=std::sin(clock_*1.55f)*.002f*(1-ads_*.8f)*motion;
        position.x-=reload*.045f+inspection*.11f;
        position.y-=reload*.055f+sprint_*.09f+settle*.32f;
        position.z+=kick_*.065f*motion+sprint_*.07f+inspection*.075f;
        const float pitch=mix(-1.5f,-2.21f,ads_)+kick_*12*motion-sprint_*20-settle*42+reload*(9+eject*30);
        const float yaw=-3*(1-ads_)+swayAngle_.y*(1-ads_*.8f)*motion+inspection*26;
        const float roll=-reload*57+inspection*52-std::sin(bobPhase_)*1.3f*walk-sprint_*12;
        Camera3D camera{{0,0,0},{0,0,-1},{0,1,0},mix(60,52,ads_),CAMERA_PERSPECTIVE};
        BeginMode3D(camera);
        BeginShaderMode(shader_);
        rlPushMatrix();
        rlTranslatef(position.x,position.y,position.z);
        rlRotatef(roll,0,0,1); rlRotatef(yaw,0,1,0); rlRotatef(pitch+swayAngle_.x*motion,1,0,0);
        DrawModel(gun_,{0,0,0},gunScale_,WHITE);
        DrawModel(hands_,{0,0,0},handScale_,WHITE);
        if(cylinder_.meshCount) {
            const Vector3 pivot{0,.0724f*gunScale_,-.1178f*gunScale_};
            rlPushMatrix();
            rlTranslatef(pivot.x-.108f*open,pivot.y+.012f*open,pivot.z+.008f*open);
            rlRotatef(open*14,0,1,0);
            rlRotatef(cylinderAngle_+(p>=0?smooth(.48f,.79f,p)*120:0),0,0,1);
            rlTranslatef(-pivot.x,-pivot.y,-pivot.z);
            DrawModel(cylinder_,{0,0,0},gunScale_,WHITE);
            drawCartridges(game,p,open,pivot);
            rlPopMatrix();
        }
        if(trigger_.meshCount) {
            const Vector3 pivot{0,.026f*gunScale_,-.093f*gunScale_};
            rlPushMatrix(); rlTranslatef(pivot.x,pivot.y,pivot.z);
            rlRotatef(kick_*-18,1,0,0); rlTranslatef(-pivot.x,-pivot.y,-pivot.z);
            DrawModel(trigger_,{0,0,0},gunScale_,WHITE); rlPopMatrix();
        }
        if(game.shotTimer>.35f) {
            const float strength=std::clamp((game.shotTimer-.35f)/.08f,0.0f,1.0f);
            const Vector3 muzzle{0,.0895f*gunScale_,-.375f*gunScale_};
            DrawSphere(muzzle,.023f*strength,{255,238,188,255});
            DrawCylinderEx(muzzle,{muzzle.x,muzzle.y,muzzle.z-.12f*strength},.027f*strength,0,7,{255,161,54,225});
        }
        rlPopMatrix();
        EndShaderMode(); EndMode3D();
    }

    void unload() {
        if(gun_.meshCount) UnloadModel(gun_);
        if(hands_.meshCount) UnloadModel(hands_);
        if(cylinder_.meshCount) UnloadModel(cylinder_);
        if(trigger_.meshCount) UnloadModel(trigger_);
        for(auto id:textures_) UnloadTexture(Texture2D{id,0,0,0,0});
        textures_.clear(); gun_={}; hands_={}; cylinder_={}; trigger_={}; ready_=false;
    }

private:
    Model gun_{},hands_{},cylinder_{},trigger_{};
    Shader shader_{};
    std::set<unsigned int> textures_;
    std::vector<Transform> restLocal_,posed_;
    bool ready_=false,wasReloading_=false,inspectHeld_=false,paraRig_=false;
    int rightWrist_=-1,leftWrist_=-1,reloadRounds_=6,spent_=6;
    float clock_=0,equip_=0,ads_=0,movement_=0,sprint_=0,bobPhase_=0,kick_=0;
    float inspectTime_=-1,reloadProgress_=-1,cylinderAngle_=0,cylinderTarget_=0;
    Vector2 sway_{},swayAngle_{};
    static constexpr float gunScale_=1.12f;
    float handScale_=.083f;
    static float mix(float a,float b,float t) { return a+(b-a)*t; }
    static float approach(float a,float b,float dt,float speed) { return mix(a,b,1-std::exp(-dt*speed)); }
    static float smooth(float a,float b,float x) {
        float t=std::clamp((x-a)/(b-a),0.0f,1.0f); return t*t*(3-2*t);
    }
    int bone(const char* name) const {
        for(int i=0;i<hands_.boneCount;++i) if(std::strcmp(hands_.bones[i].name,name)==0) return i;
        return -1;
    }
    void prepare(Model& model,bool gamma) {
        for(int m=0;m<model.materialCount;++m) {
            auto& mat=model.materials[m]; mat.shader=shader_;
            if(gamma) {
                auto& c=mat.maps[MATERIAL_MAP_DIFFUSE].color;
                auto srgb=[](unsigned char x){return static_cast<unsigned char>(255*std::pow(x/255.f,1/2.2f));};
                c={srgb(c.r),srgb(c.g),srgb(c.b),c.a};
            }
            for(int map=0;map<=MATERIAL_MAP_BRDF;++map) {
                auto texture=mat.maps[map].texture;
                if(texture.id>1) { textures_.insert(texture.id); SetTextureFilter(texture,TEXTURE_FILTER_BILINEAR); }
            }
        }
    }
    Mesh subset(const Mesh& source,const std::vector<int>& triangles) {
        Mesh mesh{}; mesh.triangleCount=static_cast<int>(triangles.size()); mesh.vertexCount=mesh.triangleCount*3;
        mesh.vertices=static_cast<float*>(MemAlloc(mesh.vertexCount*3*sizeof(float)));
        mesh.normals=static_cast<float*>(MemAlloc(mesh.vertexCount*3*sizeof(float)));
        mesh.texcoords=static_cast<float*>(MemAlloc(mesh.vertexCount*2*sizeof(float)));
        int out=0;
        for(int triangle:triangles) for(int corner=0;corner<3;++corner) {
            const int index=source.indices?source.indices[triangle*3+corner]:triangle*3+corner;
            for(int axis=0;axis<3;++axis) {
                mesh.vertices[out*3+axis]=source.vertices[index*3+axis];
                mesh.normals[out*3+axis]=source.normals?source.normals[index*3+axis]:(axis==1?1.f:0.f);
            }
            mesh.texcoords[out*2]=source.texcoords?source.texcoords[index*2]:0;
            mesh.texcoords[out*2+1]=source.texcoords?source.texcoords[index*2+1]:0;
            ++out;
        }
        UploadMesh(&mesh,false); return mesh;
    }
    void splitMechanism() {
        for(int m=0;m<gun_.meshCount;++m) {
            auto& mesh=gun_.meshes[m];
            std::vector<int> body,mechanism;
            const bool cylinder=m==3,trigger=m==2;
            if(!cylinder && !trigger) continue;
            for(int t=0;t<mesh.triangleCount;++t) {
                bool selected=true;
                for(int c=0;c<3;++c) {
                    int i=mesh.indices?mesh.indices[t*3+c]:t*3+c;
                    const float x=mesh.vertices[i*3],y=mesh.vertices[i*3+1],z=mesh.vertices[i*3+2];
                    if(cylinder) selected=selected&&std::abs(x)<.032f&&y>.039f&&y<.106f&&z>-.156f&&z<-.080f;
                    else selected=selected&&std::abs(x)<.006f&&y>-.012f&&y<.030f&&z>-.107f&&z<-.075f;
                }
                (selected?mechanism:body).push_back(t);
            }
            if(mechanism.empty()||body.empty()) continue;
            Model part=LoadModelFromMesh(subset(mesh,mechanism));
            part.materials[0].shader=shader_;
            part.materials[0].maps[MATERIAL_MAP_DIFFUSE].color=gun_.materials[gun_.meshMaterial[m]].maps[MATERIAL_MAP_DIFFUSE].color;
            Mesh replacement=subset(mesh,body); UnloadMesh(mesh); mesh=replacement;
            if(cylinder) cylinder_=part; else trigger_=part;
        }
    }
    void armOverrides(bool left,Vector3 wrist,Quaternion wristRotation,
                      std::vector<bool>& overridden) {
        const int upper=bone(paraRig_?(left?"upper_arm.L":"upper_arm.R"):(left?"bicep.l":"bicep.r"));
        const int elbow=bone(paraRig_?(left?"forearm.L":"forearm.R"):(left?"forearm.l":"forearm.r"));
        const int hand=left?leftWrist_:rightWrist_;
        const int shoulder=bone(paraRig_?(left?"clavicle.L":"clavicle.R"):(left?"shoulder.l":"shoulder.r"));
        if(upper<0||elbow<0||hand<0||shoulder<0) return;
        const float side=left?-1.f:1.f;
        const float reload=reloadProgress_>=0?smooth(0,.15f,reloadProgress_)*(1-smooth(.82f,1,reloadProgress_)):0;
        const float inspection=inspectTime_>=0?std::sin(std::clamp(inspectTime_/2.45f,0.0f,1.0f)*PI):0;
        const float counterRoll=(reload*57-inspection*52+sprint_*12)*DEG2RAD;
        Vector3 start=Vector3RotateByAxisAngle({side*.27f,-.255f,.255f},{0,0,1},counterRoll);
        const float a=Vector3Distance(hands_.bindPose[upper].translation,hands_.bindPose[elbow].translation)*handScale_;
        const float b=Vector3Distance(hands_.bindPose[elbow].translation,hands_.bindPose[hand].translation)*handScale_;
        Vector3 axis=Vector3Normalize(Vector3Subtract(wrist,start));
        const float distance=std::clamp(Vector3Distance(wrist,start),.04f,a+b-.003f);
        start=Vector3Subtract(wrist,Vector3Scale(axis,distance));
        const float along=(a*a-b*b+distance*distance)/(2*distance);
        const float height=std::sqrt(std::max(0.f,a*a-along*along));
        Vector3 bend=Vector3Normalize(Vector3CrossProduct(axis,{0,1,0}));
        if((left&&bend.x>0)||(!left&&bend.x<0)) bend=Vector3Negate(bend);
        Vector3 hinge=Vector3Add(Vector3Add(start,Vector3Scale(axis,along)),Vector3Scale(bend,height));
        auto set=[&](int boneIndex,Vector3 target,Vector3 originalDirection,Vector3 targetDirection) {
            posed_[boneIndex]=hands_.bindPose[boneIndex];
            posed_[boneIndex].translation=Vector3Scale(target,1/handScale_);
            const Quaternion delta=QuaternionFromVector3ToVector3(Vector3Normalize(originalDirection),Vector3Normalize(targetDirection));
            posed_[boneIndex].rotation=QuaternionMultiply(delta,hands_.bindPose[boneIndex].rotation);
            overridden[boneIndex]=true;
        };
        const Vector3 shoulderPosition=Vector3RotateByAxisAngle({side*.35f,-.38f,.43f},{0,0,1},counterRoll);
        set(shoulder,shoulderPosition,Vector3Subtract(hands_.bindPose[upper].translation,hands_.bindPose[shoulder].translation),Vector3Subtract(start,shoulderPosition));
        set(upper,start,Vector3Subtract(hands_.bindPose[elbow].translation,hands_.bindPose[upper].translation),Vector3Subtract(hinge,start));
        set(elbow,hinge,Vector3Subtract(hands_.bindPose[hand].translation,hands_.bindPose[elbow].translation),Vector3Subtract(wrist,hinge));
        posed_[hand]=hands_.bindPose[hand]; posed_[hand].translation=Vector3Scale(wrist,1/handScale_);
        posed_[hand].rotation=wristRotation; overridden[hand]=true;
    }
    Quaternion gripRotation(bool left,Vector3 fingers,Vector3 indexSide) const {
        if(!paraRig_) return QuaternionFromEuler(-.14f,left?0:PI,0);
        const int middle1=bone(left?"f_middle.01.L":"f_middle.01.R");
        const int middle2=bone(left?"f_middle.02.L":"f_middle.02.R");
        const int index=bone(left?"f_index.01.L":"f_index.01.R");
        const int pinky=bone(left?"f_pinky.01.L":"f_pinky.01.R");
        const int wrist=left?leftWrist_:rightWrist_;
        if(middle1<0||middle2<0||index<0||pinky<0) return QuaternionIdentity();
        Vector3 originalFingers=Vector3Normalize(Vector3Subtract(hands_.bindPose[middle2].translation,hands_.bindPose[middle1].translation));
        Vector3 originalSide=Vector3Subtract(hands_.bindPose[index].translation,hands_.bindPose[pinky].translation);
        originalSide=Vector3Normalize(Vector3Subtract(originalSide,Vector3Scale(originalFingers,Vector3DotProduct(originalSide,originalFingers))));
        fingers=Vector3Normalize(fingers);
        indexSide=Vector3Normalize(Vector3Subtract(indexSide,Vector3Scale(fingers,Vector3DotProduct(indexSide,fingers))));
        const Quaternion swing=QuaternionFromVector3ToVector3(originalFingers,fingers);
        const Vector3 side=Vector3RotateByQuaternion(originalSide,swing);
        const float twist=std::atan2(Vector3DotProduct(Vector3CrossProduct(side,indexSide),fingers),Vector3DotProduct(side,indexSide));
        return QuaternionMultiply(QuaternionMultiply(QuaternionFromAxisAngle(fingers,twist),swing),hands_.bindPose[wrist].rotation);
    }
    void poseHands() {
        if(!ready_) return;
        const float p=reloadProgress_;
        const float reaching=p>=0?smooth(.07f,.30f,p)*(1-smooth(.79f,.97f,p)):0;
        const float loading=p>=0?smooth(.43f,.61f,p)*(1-smooth(.70f,.82f,p)):0;
        // The palm runs toward the muzzle; the index knuckle sits above the
        // pinky. Keeping that anatomical basis avoids the old open-claw grip.
        Vector3 right{.023f,-.050f,.073f};
        Vector3 left{-mix(.035f,.150f,reaching),mix(-.067f,.040f,reaching),mix(.111f,-.065f,reaching)};
        left.z+=loading*.075f; left.y-=loading*.012f;
        Quaternion rightRotation=gripRotation(false,{0,0,-1},{0,1,0});
        Quaternion leftRotation=gripRotation(true,{mix(.12f,.25f,reaching),0,-1},{0,1,0});
        leftRotation=QuaternionMultiply(QuaternionFromEuler(reaching*.35f,reaching*-.20f,reaching*-.25f),leftRotation);
        std::vector<bool> overridden(hands_.boneCount,false);
        armOverrides(false,right,rightRotation,overridden); armOverrides(true,left,leftRotation,overridden);
        for(int i=0;i<hands_.boneCount;++i) {
            if(overridden[i]) continue;
            Transform local=restLocal_[i];
            const std::string name=hands_.bones[i].name;
            if(name.find("finger_")==0 || name.find("f_")==0 || name.find("thumb.")==0) {
                const bool leftHand=name.find(paraRig_?".L":".l")!=std::string::npos;
                const bool thumb=name.find("thumb")!=std::string::npos;
                const bool index=name.find("index")!=std::string::npos;
                const bool proximal=name.find(".01")!=std::string::npos||name.find("1.")!=std::string::npos;
                float curl=thumb?(proximal?.50f:.24f):index?(proximal?.10f:.50f):(proximal?.85f:1.10f);
                if(leftHand) curl*=mix(1,.55f,reaching);
                if(!leftHand && index) curl+=kick_*.12f;
                // para's phalanges bend around positive local X. Mirrored
                // wrist bases provide the opposing left/right grip direction.
                local.rotation=QuaternionMultiply(local.rotation,QuaternionFromAxisAngle(paraRig_?Vector3{1,0,0}:Vector3{0,0,1},paraRig_?curl:leftHand?-curl:curl));
            }
            const int parent=hands_.bones[i].parent;
            if(parent>=0) {
                const auto& base=posed_[parent];
                posed_[i].translation=Vector3Add(base.translation,Vector3RotateByQuaternion(Vector3Multiply(local.translation,base.scale),base.rotation));
                posed_[i].rotation=QuaternionMultiply(base.rotation,local.rotation);
                posed_[i].scale=Vector3Multiply(base.scale,local.scale);
            } else posed_[i]=local;
        }
        Transform* frame=posed_.data();
        ModelAnimation animation{}; animation.boneCount=hands_.boneCount; animation.frameCount=1;
        animation.bones=hands_.bones; animation.framePoses=&frame;
        UpdateModelAnimation(hands_,animation,0);
    }
    void drawCartridges(const Game& game,float p,float open,Vector3 pivot) const {
        if(open<.01f) return;
        for(int i=0;i<6;++i) {
            const float angle=i*PI/3;
            Vector3 at{pivot.x+std::cos(angle)*.0205f,pivot.y+std::sin(angle)*.0205f,pivot.z+.035f};
            if(p>.28f && p<.55f && i<spent_) {
                const float eject=(p-.28f)/.27f;
                at.z+=eject*.19f; at.y-=eject*eject*.16f;
                at.x+=std::sin(i*2.f)*eject*.035f;
                DrawCylinderEx(at,{at.x,at.y,at.z+.030f},.0047f,.0047f,8,{166,117,49,255});
            }
            if(p>.52f && p<.82f && i<reloadRounds_) {
                const float insertion=1-smooth(.54f,.76f,p);
                at.z+=.072f*insertion;
                DrawCylinderEx(at,{at.x,at.y,at.z+.030f},.0048f,.0048f,8,{195,151,73,255});
            } else if(p>.76f || i>=spent_) {
                DrawCylinderEx(at,{at.x,at.y,at.z+.006f},.0046f,.0046f,8,{153,111,49,255});
            }
        }
        if(p>.50f && p<.79f && reloadRounds_>0) {
            const float insertion=1-smooth(.54f,.76f,p);
            const float withdrawal=smooth(.74f,.79f,p);
            Vector3 cap{pivot.x,pivot.y,pivot.z+.069f+.072f*insertion+.06f*withdrawal};
            DrawCylinderEx(cap,{cap.x,cap.y,cap.z+.012f},.029f,.029f,12,{37,40,42,255});
            DrawCylinderEx({cap.x,cap.y,cap.z+.012f},{cap.x,cap.y,cap.z+.032f},.009f,.009f,10,{60,64,66,255});
        }
        (void)game;
    }
};

} // namespace nf
