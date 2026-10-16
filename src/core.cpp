#include "core.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>

namespace nf {
namespace {
constexpr float pi = 3.14159265359f;
float lengthXZ(Vec3 a, Vec3 b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}
float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec3 minus(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
Vec3 normalized(Vec3 v) {
    const float n = std::sqrt(dot(v, v));
    return n > .00001f ? Vec3{v.x/n, v.y/n, v.z/n} : Vec3{};
}
float rayBox(Vec3 origin, Vec3 direction, const Box& box, float limit) {
    float near = 0, far = limit;
    const std::array<float,3> o{origin.x,origin.y,origin.z};
    const std::array<float,3> d{direction.x,direction.y,direction.z};
    const std::array<float,3> c{box.center.x,box.center.y,box.center.z};
    const std::array<float,3> s{box.size.x,box.size.y,box.size.z};
    for (int axis=0; axis<3; ++axis) {
        const float low=c[axis]-s[axis]*.5f, high=c[axis]+s[axis]*.5f;
        if (std::abs(d[axis]) < .000001f) {
            if (o[axis] < low || o[axis] > high) return limit;
        } else {
            float a=(low-o[axis])/d[axis], b=(high-o[axis])/d[axis];
            if (a>b) std::swap(a,b);
            near=std::max(near,a); far=std::min(far,b);
            if (near>far) return limit;
        }
    }
    return near <= limit && far >= 0 ? near : limit;
}
}

Game::Game(std::uint32_t seed) : seed_(seed), rng_(seed) { reset(); }

void Game::reset() {
    rng_.seed(seed_);
    walls.clear(); enemies.clear(); pickups.clear(); rooms.clear();
    player={0,1.65f,11.5f}; exitPosition={0,0,12.6f};
    yaw=0; pitch=0; health=100; sanity=100; stamina=100; battery=100;
    time=0; shotTimer=0; reloadTimer=0; damageFlash=0;
    ammo=6; reserve=36; tapes=0; kills=0;
    flashlight=true; won=false; dead=false;
    firedThisFrame=false; hitThisFrame=false; shotEnd=player; horrorPulse=0;
    fireHeld_=false; reloadHeld_=false; interactHeld_=false; flashlightHeld_=false;
    eventTimer_=14;
    auto wall=[this](float x,float z,float width,float depth,float height=3.8f) {
        walls.push_back({{x,height*.5f,z},{width,height,depth}});
    };
    wall(-10.2f,-4.5f,.4f,37.8f); wall(10.2f,-4.5f,.4f,37.8f);
    wall(0,-23.2f,20.8f,.4f); wall(0,14.2f,20.8f,.4f);
    for (float x : {-2.0f,2.0f}) {
        wall(x,-20.6f,.30f,5.2f);
        wall(x,-11.0f,.30f,9.6f);
        wall(x,1.0f,.30f,9.6f);
        wall(x,11.2f,.30f,6.0f);
    }
    for (float z : {1.0f,-11.0f}) {
        wall(-6.0f,z,8.0f,.30f); wall(6.0f,z,8.0f,.30f);
    }
    // Low concrete plinths create cover without sealing any room or doorway.
    wall(-6.5f,8.4f,2.2f,1.0f,1.15f);
    wall(7.0f,4.2f,2.5f,1.2f,1.15f);
    wall(-6.0f,-7.8f,2.2f,1.0f,1.15f);
    wall(6.6f,-3.1f,2.2f,1.0f,1.15f);
    wall(-6.8f,-19.5f,2.2f,1.0f,1.15f);
    wall(6.6f,-19.8f,2.2f,1.0f,1.15f);
    rooms={{{-6,0,7},"ПРОХОДНАЯ",101},{{6,0,7},"ГЕНЕРАТОРНАЯ",102},
           {{-6,0,-5},"АРХИВ",201},{{6,0,-5},"АКУСТИКА",202},
           {{-6,0,-17},"ЭХО-КАМЕРА",301},{{6,0,-17},"ПУЛЬТОВАЯ",302}};
    pickups={{{-8.2f,.9f,5.6f},PickupType::Tape,false},
             {{8.1f,.9f,-7.0f},PickupType::Tape,false},
             {{-8.1f,.9f,-16.7f},PickupType::Tape,false},
             {{6.0f,.9f,9.8f},PickupType::Ammo,false},
             {{-7.5f,.9f,-3.0f},PickupType::Ammo,false},
             {{8.0f,.9f,-16.0f},PickupType::Ammo,false},
             {{4.5f,.9f,4.0f},PickupType::Battery,false},
             {{-4.3f,.9f,-9.0f},PickupType::Medkit,false},
             {{4.7f,.9f,-14.0f},PickupType::Medkit,false},
             {{-4.0f,.9f,-20.6f},PickupType::Battery,false}};
    auto enemy=[this](float x,float z,int kind=0) {
        Enemy e; e.position={x,0,z}; e.kind=kind;
        e.health=kind ? 1.0f : 72.0f;
        e.phase=std::uniform_real_distribution<float>(0,2*pi)(rng_);
        enemies.push_back(e);
    };
    enemy(7.6f,7.8f); enemy(-7.9f,-7.0f);
    enemy(7.4f,-18.0f); enemy(-5.0f,-21.2f);
    enemy(-8.2f,10.5f,1); enemy(8.5f,-1.8f,1); enemy(0,-21.6f,1);
    enemyWaypoints_.resize(enemies.size());
    repathTimers_.assign(enemies.size(),0);
    for (std::size_t i=0;i<enemies.size();++i) enemyWaypoints_[i]=enemies[i].position;
    say("Найдите 3 кассеты и вернитесь к лифту. Не доверяйте сигналу.",7);
}

Vec3 Game::forward() const {
    const float cp=std::cos(pitch);
    return {std::sin(yaw)*cp,std::sin(pitch),-std::cos(yaw)*cp};
}

bool Game::blocked(Vec3 p,float radius) const {
    // Treat the body as a circle, allowing smooth movement around corners.
    for (const auto& w:walls) {
        const float nx=std::clamp(p.x,w.center.x-w.size.x*.5f,w.center.x+w.size.x*.5f);
        const float nz=std::clamp(p.z,w.center.z-w.size.z*.5f,w.center.z+w.size.z*.5f);
        const float dx=p.x-nx,dz=p.z-nz;
        if (dx*dx+dz*dz < radius*radius) return true;
    }
    return p.x < -10.5f || p.x > 10.5f || p.z < -23.5f || p.z > 14.5f;
}

float Game::wallDistance(Vec3 origin,Vec3 direction,float maxDistance) const {
    direction=normalized(direction);
    if (dot(direction,direction)<.5f) return maxDistance;
    float nearest=maxDistance;
    for (const auto& w:walls) nearest=std::min(nearest,rayBox(origin,direction,w,nearest));
    return nearest;
}

const Pickup* Game::nearbyPickup() const {
    const Pickup* nearest=nullptr; float distance=1.65f;
    for (const auto& pickup:pickups) {
        if (pickup.collected) continue;
        const float d=lengthXZ(player,pickup.position);
        Vec3 p=pickup.position; p.y=1.2f;
        const Vec3 ray=minus(p,player);
        const float rayLength=std::sqrt(dot(ray,ray));
        if (d<distance && wallDistance(player,ray,rayLength+.01f)>=rayLength-.03f) {
            nearest=&pickup; distance=d;
        }
    }
    return nearest;
}

void Game::say(const std::string& text,float duration) { message=text; messageTimer=duration; }

void Game::move(Vec3& p,float dx,float dz,float radius) {
    // Subdivide independently of frame time, including unusually long external steps.
    const int steps=std::max(1,static_cast<int>(std::ceil(std::hypot(dx,dz)/.10f)));
    for(int i=0;i<steps;++i) {
        Vec3 next=p; next.x+=dx/steps;
        if(!blocked(next,radius)) p.x=next.x;
        next=p; next.z+=dz/steps;
        if(!blocked(next,radius)) p.z=next.z;
    }
}

Vec3 Game::pathDirection(Vec3 from,Vec3 target) const {
    const float d=lengthXZ(from,target);
    if(d<.01f) return {};
    bool direct=true;
    const int checks=std::max(1,static_cast<int>(std::ceil(d/.25f)));
    for(int i=1;i<=checks;++i) {
        const float t=static_cast<float>(i)/checks;
        if(blocked({from.x+(target.x-from.x)*t,0,from.z+(target.z-from.z)*t},.29f)) {
            direct=false; break;
        }
    }
    if(direct) return normalized({target.x-from.x,0,target.z-from.z});

    constexpr int nx=39,nz=73,total=nx*nz;
    auto point=[](int index) -> Vec3 { return {-9.5f+(index%nx)*.5f,0,-22.5f+(index/nx)*.5f}; };
    auto index=[](Vec3 p) {
        int x=std::clamp(static_cast<int>(std::round((p.x+9.5f)*2)),0,nx-1);
        int z=std::clamp(static_cast<int>(std::round((p.z+22.5f)*2)),0,nz-1);
        return z*nx+x;
    };
    int start=index(from), goal=index(target);
    // Rounded positions can land beside a low plinth; choose the closest free node.
    auto nearestFree=[&](int initial,Vec3 p) {
        if(!blocked(point(initial),.30f)) return initial;
        int best=initial; float closest=std::numeric_limits<float>::max();
        for(int i=0;i<total;++i) {
            const float dist=lengthXZ(point(i),p);
            if(dist<closest && !blocked(point(i),.30f)) { best=i; closest=dist; }
        }
        return best;
    };
    start=nearestFree(start,from); goal=nearestFree(goal,target);
    if(start==goal) return normalized({target.x-from.x,0,target.z-from.z});
    std::array<int,total> previous; previous.fill(-1);
    std::queue<int> queue; queue.push(start); previous[start]=start;
    while(!queue.empty() && previous[goal]==-1) {
        const int current=queue.front(); queue.pop();
        const int cx=current%nx,cz=current/nx;
        const std::array<int,4> dx{1,-1,0,0},dz{0,0,1,-1};
        for(int a=0;a<4;++a) {
            const int x=cx+dx[a],z=cz+dz[a];
            if(x<0 || x>=nx || z<0 || z>=nz) continue;
            const int next=z*nx+x;
            if(previous[next]!=-1 || blocked(point(next),.30f)) continue;
            previous[next]=current; queue.push(next);
        }
    }
    if(previous[goal]==-1) return {};
    int next=goal;
    while(previous[next]!=start && previous[next]!=next) next=previous[next];
    const Vec3 p=point(next);
    return normalized({p.x-from.x,0,p.z-from.z});
}

void Game::shoot() {
    if(shotTimer>0 || reloadTimer>0) return;
    if(ammo<=0) { say(reserve>0 ? "Барабан пуст. Нажмите R для перезарядки." : "Патронов нет.",2); return; }
    --ammo; shotTimer=.43f; firedThisFrame=true;
    const Vec3 ray=forward();
    float nearest=wallDistance(player,ray,60.0f);
    Enemy* hit=nullptr;
    for(auto& enemy:enemies) {
        if(!enemy.alive) continue;
        const Box body{{enemy.position.x,1.05f,enemy.position.z},{.82f,2.1f,.82f}};
        const float distance=rayBox(player,ray,body,nearest);
        if(distance<nearest) { nearest=distance; hit=&enemy; }
    }
    shotEnd={player.x+ray.x*nearest,player.y+ray.y*nearest,player.z+ray.z*nearest};
    if(hit) {
        hitThisFrame=true; hit->health-=36;
        if(hit->health<=0) {
            hit->alive=false;
            if(hit->kind==0) { ++kills; sanity=std::min(100.0f,sanity+3); }
            else { sanity=std::max(0.0f,sanity-2); say("Здесь никого не было.",2.8f); }
        }
    }
    // A shot carries through the facility. Dormant physical enemies wake up.
    for(auto& enemy:enemies) if(enemy.alive && enemy.kind==0) enemy.seenTime=12;
}

void Game::collect() {
    if(lengthXZ(player,exitPosition)<1.8f) {
        if(tapes>=3) {
            won=true;
            say("СИГНАЛ ЗАГЛУШЁН. Вы вспомнили своё имя. Лифт поднимается.",1000);
            return;
        }
        say("Лифт заблокирован. Найдите все кассеты ("+std::to_string(tapes)+" / 3).",3);
    }
    const Pickup* nearby=nearbyPickup();
    if(!nearby) return;
    const std::size_t index=static_cast<std::size_t>(nearby-pickups.data());
    auto& pickup=pickups[index];
    switch(pickup.type) {
    case PickupType::Ammo:
        reserve=std::min(96,reserve+12); say("Найдено 12 патронов.",2.5f); break;
    case PickupType::Medkit:
        health=std::min(100.0f,health+38); say("Аптечка. Здоровье восстановлено.",2.5f); break;
    case PickupType::Battery:
        battery=std::min(100.0f,battery+55); say("Аккумулятор. Заряд фонаря восстановлен.",2.5f); break;
    case PickupType::Tape:
        ++tapes; sanity=std::min(100.0f,sanity+20); horrorPulse=1;
        if(tapes==1) say("КАССЕТА 01: В приёмнике звучит ваш собственный голос.",6);
        else if(tapes==2) say("КАССЕТА 02: Учёные не покидали станцию. Вы тоже.",6);
        else say("КАССЕТА 03: Не отвечайте на сигнал. Возвращайтесь к лифту.",6);
        for(auto& enemy:enemies) if(enemy.kind==0) enemy.seenTime=1000;
        break;
    }
    pickup.collected=true;
}

void Game::update(float dt,const Input& input) {
    firedThisFrame=false; hitThisFrame=false;
    dt=std::clamp(dt,0.0f,.25f);
    if(!dead && !won) {
        if(input.toggleFlashlight && !flashlightHeld_) {
            if(flashlight) flashlight=false;
            else if(battery>2) flashlight=true;
            else say("Аккумулятор разряжен. Дождитесь восстановления заряда.",2);
        }
        if(input.reload && !reloadHeld_ && reloadTimer<=0 && ammo<6 && reserve>0) {
            reloadTimer=1.85f; say("Перезарядка...",1.85f);
        }
        if(input.fire && !fireHeld_) shoot();
        if(input.interact && !interactHeld_) collect();
        const int steps=std::max(1,static_cast<int>(std::ceil(dt/(1.0f/60))));
        for(int i=0;i<steps && !dead && !won;++i) step(dt/steps,input);
    }
    fireHeld_=input.fire; reloadHeld_=input.reload;
    interactHeld_=input.interact; flashlightHeld_=input.toggleFlashlight;
}

void Game::step(float dt,const Input& input) {
    time+=dt;
    shotTimer=std::max(0.0f,shotTimer-dt);
    damageFlash=std::max(0.0f,damageFlash-dt*1.8f);
    horrorPulse=std::max(0.0f,horrorPulse-dt*.35f);
    messageTimer=std::max(0.0f,messageTimer-dt);
    if(reloadTimer>0) {
        reloadTimer=std::max(0.0f,reloadTimer-dt);
        if(reloadTimer==0) {
            const int rounds=std::min(6-ammo,reserve);
            ammo+=rounds; reserve-=rounds;
        }
    }
    float f=input.forward,s=input.strafe;
    const float magnitude=std::hypot(f,s);
    if(magnitude>1) { f/=magnitude; s/=magnitude; }
    const bool running=input.sprint && magnitude>.1f && stamina>0;
    const float speed=running ? 4.6f : 2.65f;
    stamina=std::clamp(stamina+dt*(running ? -25.0f : 18.0f),0.0f,100.0f);
    move(player,(std::sin(yaw)*f+std::cos(yaw)*s)*speed*dt,
                (-std::cos(yaw)*f+std::sin(yaw)*s)*speed*dt,.23f);
    if(flashlight) {
        battery=std::max(0.0f,battery-dt*.72f);
        if(battery<=0) { flashlight=false; say("Фонарь разряжен. В темноте аккумулятор восстанавливается.",3); }
    } else battery=std::min(100.0f,battery+dt*1.05f);
    sanity=std::max(0.0f,sanity-dt*(flashlight ? .045f : .36f));

    // Public collections may be edited by tools or tests; keep path caches aligned.
    if(enemyWaypoints_.size()!=enemies.size()) {
        enemyWaypoints_.resize(enemies.size()); repathTimers_.assign(enemies.size(),0);
        for(std::size_t i=0;i<enemies.size();++i) enemyWaypoints_[i]=enemies[i].position;
    }
    for(std::size_t i=0;i<enemies.size();++i) {
        auto& enemy=enemies[i];
        if(!enemy.alive) continue;
        const float distance=lengthXZ(player,enemy.position);
        enemy.attackCooldown=std::max(0.0f,enemy.attackCooldown-dt);
        Vec3 eye{enemy.position.x,1.45f,enemy.position.z};
        const Vec3 direction=normalized(minus(eye,player));
        const float sightDistance=std::sqrt(dot(minus(eye,player),minus(eye,player)));
        const bool visible=wallDistance(player,direction,sightDistance+.02f)>=sightDistance-.05f;
        if(enemy.kind==1) {
            if(visible && distance<18 && dot(forward(),direction)>.90f) {
                enemy.seenTime+=dt;
                sanity=std::max(0.0f,sanity-dt*(flashlight ? 4.5f : 7.0f));
                horrorPulse=std::max(horrorPulse,.35f);
                if(enemy.seenTime>3.4f) { enemy.alive=false; say("Не смотрите на него.",3); }
            } else if(enemy.seenTime>.2f) {
                // An observed silhouette survives only while the observer keeps looking.
                enemy.alive=false;
                horrorPulse=std::max(horrorPulse,.45f);
            }
            continue;
        }
        if(visible && distance<15) enemy.seenTime=std::max(enemy.seenTime,8.0f);
        enemy.seenTime=std::max(0.0f,enemy.seenTime-dt);
        const bool awake=enemy.seenTime>0 || distance<6 || tapes>0;
        if(!awake) continue;
        if(visible && distance<7) sanity=std::max(0.0f,sanity-dt*.7f);
        if(distance>1.0f) {
            repathTimers_[i]-=dt;
            if(repathTimers_[i]<=0 || lengthXZ(enemy.position,enemyWaypoints_[i])<.13f) {
                const Vec3 directionToPlayer=pathDirection(enemy.position,player);
                enemyWaypoints_[i]={enemy.position.x+directionToPlayer.x*.55f,0,
                                    enemy.position.z+directionToPlayer.z*.55f};
                repathTimers_[i]=.36f;
            }
            const Vec3 directionToWaypoint=normalized(minus(enemyWaypoints_[i],enemy.position));
            const float enemySpeed=1.22f+tapes*.13f+(100-sanity)*.0025f;
            move(enemy.position,directionToWaypoint.x*enemySpeed*dt,directionToWaypoint.z*enemySpeed*dt,.29f);
        } else if(visible && enemy.attackCooldown<=0) {
            health=std::max(0.0f,health-13.0f); sanity=std::max(0.0f,sanity-4);
            damageFlash=1; enemy.attackCooldown=1.15f;
            say("ДВИГАЙТЕСЬ. Оно уже рядом.",1.5f);
        }
    }
    eventTimer_-=dt;
    if(eventTimer_<=0) {
        horrorPulse=std::max(horrorPulse,.65f);
        const std::array<const char*,5> whispers{
            "Щёлкает приёмник. Кто-то дышит в микрофон.",
            "Вы уже проходили по этому коридору.",
            "Где-то позади закрывается дверь.",
            "Сигнал знает, когда вы отводите взгляд.",
            "После вашей смены никого не будет."};
        if(messageTimer<=0) say(whispers[rng_()%whispers.size()],4);
        eventTimer_=std::uniform_real_distribution<float>(18,29)(rng_);
    }
    if(sanity<=0) { health=std::max(0.0f,health-dt*2); horrorPulse=.8f; }
    if(health<=0) {
        health=0; dead=true; say("СВЯЗЬ ПОТЕРЯНА. Станция помнит вас.",1000);
    }
}

} // namespace nf
