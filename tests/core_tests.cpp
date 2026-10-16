#include "core.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition,const std::string& description) {
    if(!condition) throw std::runtime_error(description);
}
void advance(nf::Game& game,float seconds,const nf::Input& input={}) {
    while(seconds>.00001f) {
        const float dt=std::min(seconds,.02f); game.update(dt,input); seconds-=dt;
    }
}
float distance(nf::Vec3 a,nf::Vec3 b) { return std::hypot(a.x-b.x,a.z-b.z); }
void connectivity() {
    nf::Game game;
    constexpr int nx=79,nz=147,total=nx*nz;
    auto point=[=](int i) -> nf::Vec3 { return {-9.75f+(i%nx)*.25f,0,-22.75f+(i/nx)*.25f}; };
    auto index=[=](nf::Vec3 p) {
        return static_cast<int>(std::lround((p.z+22.75f)*4))*nx+
               static_cast<int>(std::lround((p.x+9.75f)*4));
    };
    std::array<bool,total> reached{};
    std::queue<int> queue; const int start=index(game.player);
    queue.push(start); reached[start]=true;
    while(!queue.empty()) {
        const int cur=queue.front(); queue.pop();
        for(const auto [dx,dz] : std::array<std::pair<int,int>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}) {
            const int x=cur%nx+dx,z=cur/nx+dz;
            if(x<0 || x>=nx || z<0 || z>=nz) continue;
            const int next=z*nx+x;
            if(reached[next] || game.blocked(point(next),.30f)) continue;
            reached[next]=true; queue.push(next);
        }
    }
    for(const auto& pickup:game.pickups) {
        check(!game.blocked(pickup.position,.30f),"pickup intersects geometry");
        check(reached[index(pickup.position)],"pickup is not reachable from elevator");
    }
    for(const auto& room:game.rooms) check(reached[index(room.center)],"room is unreachable");
    check(reached[index(game.exitPosition)],"exit is unreachable");
    check(!game.blocked({2,0,7}),"door opening is blocked");
    check(game.blocked({2,0,11}),"solid corridor wall permits passage");
}
void weapons() {
    nf::Game game; game.enemies.clear();
    nf::Enemy enemy; enemy.position={4,0,11.5f};
    game.enemies.push_back(enemy);
    game.yaw=1.57079632679f;
    nf::Input fire; fire.fire=true;
    game.update(0,fire);
    check(game.firedThisFrame && !game.hitThisFrame,"shot incorrectly penetrates wall");
    check(game.enemies[0].health==72,"occluded enemy took damage");
    check(std::abs(game.wallDistance(game.player,{1,0,0},60)-1.85f)<.01f,"wall ray distance incorrect");
    game.reset(); game.enemies.clear();
    enemy.position={0,0,7}; game.enemies.push_back(enemy);
    game.update(0,fire);
    check(game.hitThisFrame && game.enemies[0].health==36 && game.ammo==5,"first shot misses visible target");
    advance(game,.5f,fire);
    check(game.ammo==5,"semi automatic weapon fires repeatedly while held");
    game.update(0,{}); game.update(0,fire);
    check(!game.enemies[0].alive && game.kills==1 && game.ammo==4,"second shot does not kill target");
    game.update(0,{});
    nf::Input reload; reload.reload=true;
    game.update(0,reload);
    check(game.reloadTimer>1.8f && game.ammo==4,"reload did not start with delay");
    game.update(0,fire);
    check(!game.firedThisFrame && game.ammo==4,"weapon fires during reload");
    advance(game,1.9f);
    check(game.ammo==6 && game.reserve==34 && game.reloadTimer==0,"reload did not transfer exact rounds");
    game.ammo=0; game.reserve=2; game.update(0,reload); advance(game,2);
    check(game.ammo==2 && game.reserve==0,"partial reload creates ammunition");
}
void movementAndResources() {
    nf::Game game; game.enemies.clear();
    nf::Input input; input.strafe=1;
    advance(game,3,input);
    check(game.player.x<1.64f && game.player.x>1.5f,"player walks through wall or stops too early");
    game.player={0,1.65f,7}; advance(game,1.5f,input);
    check(game.player.x>3,"player cannot pass through door");
    input={}; input.forward=1; input.sprint=true;
    advance(game,1,input);
    check(game.stamina<76 && game.stamina>74,"sprinting does not consume stamina");
    advance(game,1); check(game.stamina>92,"stamina does not recover");
    const float charge=game.battery;
    advance(game,2); check(game.battery<charge,"flashlight does not consume battery");
    input={}; input.toggleFlashlight=true; game.update(0,input);
    const float low=game.battery; advance(game,2);
    check(!game.flashlight && game.battery>low,"flashlight does not recharge when off");
    game.health=34; game.battery=12; game.reserve=0;
    for(auto& pickup:game.pickups) {
        if(pickup.type==nf::PickupType::Tape) continue;
        game.player={pickup.position.x,1.65f,pickup.position.z};
        game.update(0,{}); input={}; input.interact=true; game.update(0,input);
        check(pickup.collected,"supply could not be collected");
    }
    check(game.health>34 && game.battery>12 && game.reserve>0,"supplies have no effect");
}
void objectivesAndReset() {
    nf::Game game; game.enemies.clear();
    nf::Input use; use.interact=true;
    game.update(0,use);
    check(!game.won,"exit permits completion without tapes");
    for(auto& pickup:game.pickups) {
        if(pickup.type!=nf::PickupType::Tape) continue;
        game.player={pickup.position.x,1.65f,pickup.position.z};
        game.update(0,{}); game.update(0,use);
        check(pickup.collected,"memory tape cannot be collected");
    }
    check(game.tapes==3 && !game.won,"tape counter or win gate incorrect");
    game.player={game.exitPosition.x,1.65f,game.exitPosition.z};
    game.update(0,{}); game.update(0,use);
    check(game.won,"returning to elevator does not win");
    const auto finishedPlayer=game.player;
    nf::Input forward; forward.forward=1; advance(game,1,forward);
    check(distance(game.player,finishedPlayer)<.001f,"completed game continues simulation");
    game.reset();
    check(!game.won && !game.dead && game.tapes==0 && game.health==100 && game.ammo==6,"reset leaves stale state");
    check(game.enemies.size()==7 && game.pickups.size()==10,"reset duplicates or loses entities");
    for(const auto& pickup:game.pickups) check(!pickup.collected,"reset leaves collected items");
}
void enemiesAndDeterminism() {
    nf::Game first(101),second(101);
    check(first.enemies[0].phase==second.enemies[0].phase,"seed does not reproduce entity variation");
    advance(first,22); advance(second,22);
    check(first.message==second.message && std::abs(first.sanity-second.sanity)<.0001f,"simulation is nondeterministic");
    nf::Game game; game.enemies.clear(); game.player={0,1.65f,7};
    nf::Enemy enemy; enemy.position={7.5f,0,2.4f}; enemy.seenTime=100;
    game.enemies.push_back(enemy);
    advance(game,11);
    check(distance(game.enemies[0].position,game.player)<1.2f,"enemy cannot route through room door");
    check(game.health<100,"enemy in melee range does no damage");
    game.health=1; game.enemies[0].attackCooldown=0; advance(game,.1f);
    check(game.dead && game.health==0,"lethal enemy attack does not end game");
    game.reset(); game.enemies.clear(); game.sanity=1;
    enemy.position={0,0,7}; enemy.kind=1; enemy.seenTime=0; enemy.alive=true;
    game.enemies.push_back(enemy);
    advance(game,.5f);
    check(game.sanity<1 && game.horrorPulse>0,"observed apparition has no psychological effect");
    check(game.enemies[0].alive,"apparition vanishes before gaze changes");
    game.yaw=1.57079632679f;
    advance(game,.1f);
    check(!game.enemies[0].alive,"observed apparition remains after player looks away");
    game.reset(); game.enemies.clear();
    enemy={}; enemy.position={0,0,7}; enemy.kind=1;
    game.enemies.push_back(enemy);
    game.yaw=1.57079632679f;
    advance(game,.5f);
    check(game.enemies[0].alive,"unobserved apparition vanishes without being seen");
    game.reset(); game.enemies.clear();
    enemy={}; enemy.position={4,0,11.5f}; enemy.seenTime=100;
    game.enemies.push_back(enemy);
    advance(game,1);
    check(game.health==100,"enemy attacks through concrete wall");
}

void fullPlaythrough() {
    // Exercise the shipped level without removing enemies, granting ammunition,
    // teleporting, or changing health. Aim is automated; shots are deliberately
    // spaced one second apart to include pursuit and reload pressure.
    nf::Game game;
    const std::array<nf::Vec3,16> route{{
        {0,0,7},{-3,0,7},{-4.5f,0,5.6f},{-8.2f,0,5.6f},
        {-3,0,7},{0,0,7},{0,0,-5},{3,0,-5},{8.1f,0,-7},
        {3,0,-5},{0,0,-5},{0,0,-17},{-3,0,-17},{-8.1f,0,-16.7f},
        {0,0,-17},{0,0,12.6f}
    }};
    std::size_t checkpoint=0;
    bool firedLastFrame=false;
    float nextShot=0;
    int firedRounds=0;
    for(int frame=0;frame<7500 && !game.dead && !game.won;++frame) {
        while(checkpoint+1<route.size() && distance(game.player,route[checkpoint])<.22f) ++checkpoint;
        const auto destination=route[checkpoint];
        float dx=destination.x-game.player.x,dz=destination.z-game.player.z;
        const float length=std::hypot(dx,dz);
        if(length>.001f) { dx/=length; dz/=length; }
        const nf::Enemy* target=nullptr;
        float nearest=14;
        for(const auto& enemy:game.enemies) {
            if(!enemy.alive || enemy.kind!=0) continue;
            const float d=distance(game.player,enemy.position);
            const nf::Vec3 ray{enemy.position.x-game.player.x,1.55f-game.player.y,
                               enemy.position.z-game.player.z};
            const float rayLength=std::sqrt(ray.x*ray.x+ray.y*ray.y+ray.z*ray.z);
            if(d<nearest && game.wallDistance(game.player,ray,rayLength+.01f)>=rayLength-.01f) {
                target=&enemy; nearest=d;
            }
        }
        if(target) {
            game.yaw=std::atan2(target->position.x-game.player.x,-(target->position.z-game.player.z));
            game.pitch=std::atan2(1.55f-game.player.y,nearest);
        } else { game.yaw=std::atan2(dx,-dz); game.pitch=0; }
        nf::Input input;
        input.forward=dx*std::sin(game.yaw)-dz*std::cos(game.yaw);
        input.strafe=dx*std::cos(game.yaw)+dz*std::sin(game.yaw);
        input.fire=target && game.ammo>0 && game.shotTimer<=0 && game.reloadTimer<=0 &&
                   !firedLastFrame && game.time>=nextShot;
        firedLastFrame=input.fire;
        input.reload=game.ammo<6 && game.reserve>0 && game.reloadTimer<=0 && (!target || game.ammo==0);
        input.interact=frame%2==0;
        game.update(.02f,input);
        if(game.firedThisFrame) { ++firedRounds; nextShot=game.time+1; }
    }
    check(game.won && game.tapes==3,"shipped level cannot be completed with active enemies");
    check(game.kills==4,"complete playthrough bypasses a physical enemy");
    check(firedRounds==8 && game.ammo+game.reserve>=22,"revolver balance or ammunition accounting regressed");
    check(game.health>0 && game.battery>0,"full playthrough runs out of essential resources");
}
}

int main() {
    try {
        connectivity(); weapons(); movementAndResources(); objectivesAndReset(); enemiesAndDeterminism();
        fullPlaythrough();
        std::cout << "All NULL FREQUENCY core tests passed.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr << "Core test failed: " << error.what() << '\n';
        return 1;
    }
}
