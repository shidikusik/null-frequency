#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace nf {

struct Vec3 { float x = 0, y = 0, z = 0; };
struct Box { Vec3 center, size; };

struct Enemy {
    Vec3 position;
    float health = 72;
    float attackCooldown = 0;
    bool alive = true;
    float phase = 0;
    int kind = 0; // 0: physical, 1: an apparition that drains sanity while observed.
    float seenTime = 0;
};

enum class PickupType { Ammo, Medkit, Battery, Tape };
struct Pickup {
    Vec3 position;
    PickupType type = PickupType::Ammo;
    bool collected = false;
};

struct Room { Vec3 center; std::string label; int number = 0; };
struct Input {
    float forward = 0, strafe = 0;
    bool sprint = false, fire = false, reload = false;
    bool interact = false, toggleFlashlight = false;
};

class Game {
public:
    std::vector<Box> walls;
    std::vector<Enemy> enemies;
    std::vector<Pickup> pickups;
    std::vector<Room> rooms;
    Vec3 player, exitPosition;
    float yaw = 0, pitch = 0;
    float health = 100, sanity = 100, stamina = 100, battery = 100;
    float time = 0, shotTimer = 0, reloadTimer = 0, damageFlash = 0;
    int ammo = 6, reserve = 36, tapes = 0, kills = 0;
    bool flashlight = true, won = false, dead = false;
    std::string message;
    float messageTimer = 0;

    bool firedThisFrame = false, hitThisFrame = false;
    Vec3 shotEnd;
    float horrorPulse = 0;

    explicit Game(std::uint32_t seed = 0x4e554c4cU);
    void reset();
    void update(float dt, const Input& input);
    Vec3 forward() const;
    bool blocked(Vec3 position, float radius = .22f) const;
    float wallDistance(Vec3 origin, Vec3 direction, float maxDistance) const;
    const Pickup* nearbyPickup() const;

private:
    std::uint32_t seed_;
    std::mt19937 rng_;
    bool fireHeld_ = false, reloadHeld_ = false;
    bool interactHeld_ = false, flashlightHeld_ = false;
    float eventTimer_ = 14;
    std::vector<Vec3> enemyWaypoints_;
    std::vector<float> repathTimers_;
    void step(float dt, const Input& input);
    void move(Vec3& position, float dx, float dz, float radius);
    Vec3 pathDirection(Vec3 from, Vec3 target) const;
    void shoot();
    void collect();
    void say(const std::string& text, float duration = 4);
};

} // namespace nf
