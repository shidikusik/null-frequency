#pragma once

#include <memory>

namespace nf {

// Every sound is synthesized in memory. An unavailable audio device is allowed:
// all public methods then become harmless no-ops.
class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool init();
    void update(float dt, float tension, bool moving, bool sprinting, bool flashlightOn);
    // World-relative threat: 0 = left, 1 = right; proximity fades from 0 to 1.
    void threat(float pan, float proximity);
    void shoot();
    void reload();
    void pickup();
    void hurt();
    void whisper();
    void shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace nf
