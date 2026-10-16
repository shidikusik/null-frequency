#include "audio.hpp"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace nf {
namespace {

constexpr int kSampleRate = 44100;
constexpr int kStreamFrames = 2048;
constexpr double kTau = 6.28318530717958647692;

class Noise {
public:
    explicit Noise(std::uint32_t seed = 0x9351a7u) : state_(seed) {}
    float next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return static_cast<float>(state_ & 0x00ffffffu)/8388607.5f - 1.0f;
    }
private:
    std::uint32_t state_;
};

float sine(double phase) { return static_cast<float>(std::sin(kTau*phase)); }

float decay(float time, float speed) {
    return time < 0.0f ? 0.0f : std::exp(-time*speed);
}

float click(float time, float start, float frequency, Noise& noise, float amplitude) {
    const float t = time - start;
    if (t < 0.0f) return 0.0f;
    return amplitude*decay(t, 70.0f)*(0.55f*noise.next() + 0.45f*sine(t*frequency));
}

float metalTick(float time, float start, float frequency, Noise& noise, float amplitude) {
    const float t = time - start;
    if (t < 0.0f) return 0.0f;
    const float ring = 0.55f*sine(t*frequency) + 0.25f*sine(t*frequency*2.713f)
                     + 0.12f*sine(t*frequency*4.083f);
    return amplitude*(ring*decay(t, 32.0f) + 0.18f*noise.next()*decay(t, 95.0f));
}

enum class Effect { Shot, Reload, Pickup, Hurt, Whisper, Step };

Sound synthesize(Effect effect, int variant = 0) {
    float duration = 0.25f;
    switch (effect) {
        case Effect::Shot: duration = 0.85f; break;
        case Effect::Reload: duration = 1.85f; break;
        case Effect::Pickup: duration = 0.45f; break;
        case Effect::Hurt: duration = 0.52f; break;
        case Effect::Whisper: duration = 1.85f; break;
        case Effect::Step: duration = 0.23f; break;
    }

    const int frameCount = static_cast<int>(duration*kSampleRate);
    std::vector<std::int16_t> pcm(static_cast<std::size_t>(frameCount));
    Noise noise(0x9351a7u + static_cast<std::uint32_t>(variant*9173 + static_cast<int>(effect)*73));
    float low = 0.0f;
    float band = 0.0f;
    for (int i = 0; i < frameCount; ++i) {
        const float t = static_cast<float>(i)/kSampleRate;
        const float random = noise.next();
        low += 0.09f*(random - low);
        band += 0.48f*(random - band);
        float sample = 0.0f;

        switch (effect) {
            case Effect::Shot: {
                // Separate muzzle crack, chesty barrel report, sub-bass body,
                // and filtered air. Small delays turn that impulse into a room.
                const float high = random - band;
                const float crack = (0.33f*random + 0.16f*high)*decay(t, 112.0f);
                const float body = (0.34f*low + 0.23f*sine(t*105.0f - t*t*40.0f))*decay(t, 17.0f);
                const float lowReport = 0.12f*sine(t*53.0f)*decay(t, 10.0f);
                const float roomAir = 0.10f*low*decay(t, 6.0f);
                sample = crack + body + lowReport + roomAir;
                sample += click(t, 0.018f, 1730.0f, noise, 0.045f);
                const auto reflection = [&](float delay) {
                    const int previous = i - static_cast<int>(delay*kSampleRate);
                    return previous >= 0 ? static_cast<float>(pcm[static_cast<std::size_t>(previous)])/32767.0f : 0.0f;
                };
                sample += 0.19f*reflection(0.039f) + 0.12f*reflection(0.081f)
                        + 0.075f*reflection(0.137f) + 0.045f*reflection(0.211f);
                // Saturation adds weight while keeping transients moderate.
                sample = 0.72f*std::tanh(sample*1.45f);
                break;
            }
            case Effect::Reload: {
                // 1.85 s viewmodel timeline: open, eject, insert six, close.
                sample = click(t, 0.05f, 1550.0f, noise, 0.17f)
                       + click(t, 0.24f, 970.0f, noise, 0.14f)
                       + click(t, 0.55f, 1260.0f, noise, 0.12f);
                constexpr std::array<float, 6> drops{0.59f, 0.64f, 0.69f, 0.74f, 0.81f, 0.88f};
                for (int casing = 0; casing < 6; ++casing) {
                    const float frequency = 2390.0f + casing*173.0f;
                    sample += metalTick(t, drops[static_cast<std::size_t>(casing)], frequency, noise, 0.050f);
                    sample += metalTick(t, drops[static_cast<std::size_t>(casing)] + 0.069f,
                                        frequency*1.13f, noise, 0.017f);
                    sample += click(t, 1.00f + casing*0.08f, 1830.0f + casing*91.0f, noise, 0.085f);
                }
                sample += click(t, 1.61f, 760.0f, noise, 0.23f)
                        + metalTick(t, 1.62f, 1480.0f, noise, 0.055f)
                        + click(t, 1.74f, 2040.0f, noise, 0.12f);
                break;
            }
            case Effect::Pickup: {
                const float envelope = std::min(1.0f, t*80.0f)*decay(t, 8.5f);
                sample = 0.12f*envelope*(sine(t*523.25f) + 0.55f*sine(t*783.99f));
                sample += click(t, 0.0f, 3100.0f, noise, 0.045f);
                break;
            }
            case Effect::Hurt: {
                const float attack = std::min(1.0f, t*120.0f);
                sample = attack*decay(t, 8.0f)*(0.24f*low + 0.16f*sine(90.0f*t - 35.0f*t*t));
                break;
            }
            case Effect::Whisper: {
                // Formant-shaped breath with syllabic movement, not recorded speech.
                const float fade = std::min(1.0f, t*4.0f)*std::min(1.0f, (duration - t)*3.0f);
                const float syllable = 0.28f + 0.72f*std::pow(0.5f + 0.5f*sine(t*2.4f), 2.0f);
                const float formant = 0.5f + 0.26f*sine(t*620.0f) + 0.24f*sine(t*1170.0f);
                sample = (band - low)*formant*syllable*fade*0.19f;
                break;
            }
            case Effect::Step: {
                const float pitch = 1.0f + static_cast<float>(variant)*0.045f;
                sample = (0.50f*low + 0.085f*sine(t*94.0f*pitch))*decay(t, 24.0f);
                sample += 0.12f*(band - low)*decay(t, 65.0f);
                break;
            }
        }

        // Tiny fades prevent clicks at buffer boundaries, including the shot.
        sample *= std::min(1.0f, t*2400.0f)*std::min(1.0f, (duration - t)*120.0f);
        pcm[static_cast<std::size_t>(i)] = static_cast<std::int16_t>(std::clamp(sample, -0.88f, 0.88f)*32767.0f);
    }

    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frameCount);
    wave.sampleRate = kSampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = pcm.data();
    // LoadSoundFromWave copies PCM into the device's managed audio buffer.
    return LoadSoundFromWave(wave);
}

void play(Sound sound, float volume = 1.0f, float pitch = 1.0f, float pan = 0.5f) {
    if (!IsSoundValid(sound)) return;
    SetSoundVolume(sound, volume);
    SetSoundPitch(sound, pitch);
    SetSoundPan(sound, pan);
    PlaySound(sound);
}

} // namespace

struct Audio::Impl {
    bool ready = false;
    bool ownsDevice = false;
    AudioStream drone{};
    std::array<Sound, 3> shots{};
    std::array<Sound, 4> steps{};
    Sound reloadSound{};
    Sound pickupSound{};
    Sound hurtSound{};
    Sound whisperSound{};
    int shotIndex = 0;
    int stepIndex = 0;
    float stepClock = 0.0f;
    float tension = 0.0f;
    float targetTension = 0.0f;
    float lightHum = 0.0f;
    float targetLightHum = 0.0f;
    float threatPan = 0.5f;
    float targetThreatPan = 0.5f;
    float threatProximity = 0.0f;
    float targetThreatProximity = 0.0f;
    float threatAirLow = 0.0f;
    float threatAirHigh = 0.0f;
    double threatPhase = 0.0;
    double audioTime = 0.0;
    float airNoise = 0.0f;
    Noise noise;
    std::array<std::int16_t, kStreamFrames*2> streamBuffer{};

    void fillDrone() {
        for (int i = 0; i < kStreamFrames; ++i) {
            tension += (targetTension - tension)*0.000028f;
            lightHum += (targetLightHum - lightHum)*0.00007f;
            threatPan += (targetThreatPan - threatPan)*0.00020f;
            threatProximity += (targetThreatProximity - threatProximity)*0.00012f;
            const double t = audioTime;
            const float breath = 0.78f + 0.22f*sine(t*0.085);
            const float rumble = 0.026f*sine(t*43.0)
                               + 0.018f*sine(t*64.73)
                               + 0.009f*sine(t*(87.1 + tension*0.6));
            const float distant = (0.006f + tension*0.010f)*sine(t*211.0 + 0.5*sine(t*0.17));
            airNoise += 0.025f*(noise.next() - airNoise);
            const float air = airNoise*(0.022f + tension*0.032f);
            const float electric = lightHum*0.006f*(sine(t*100.0) + 0.3f*sine(t*200.0));
            const double beatPhase = std::fmod(t*(0.77 + tension*0.67), 1.0);
            const float beat = static_cast<float>(std::exp(-beatPhase*26.0)
                + (beatPhase > 0.19 ? 0.60*std::exp(-(beatPhase - 0.19)*34.0) : 0.0));
            const float heartbeat = tension*0.035f*beat*sine(t*58.0);
            const float common = breath*rumble + distant + air + electric + heartbeat;
            const float stereo = 0.004f*sine(t*0.13)*sine(t*139.6);

            // A nearby physical enemy has its own quiet breath/growl. Equal-power
            // stereo panning preserves level while the player turns toward it.
            const float threatNoise = noise.next();
            threatAirLow += 0.012f*(threatNoise - threatAirLow);
            threatAirHigh += 0.20f*(threatNoise - threatAirHigh);
            const float breathing = 0.28f + 0.72f*std::pow(0.5f + 0.5f*sine(t*0.39), 2.0f);
            const float growl = 0.38f*sine(threatPhase) + 0.16f*sine(threatPhase*2.0);
            const float threatSound = 0.045f*threatProximity*breathing*
                (growl + 0.70f*(threatAirHigh - threatAirLow));
            const float leftThreat = threatSound*std::cos(threatPan*static_cast<float>(kTau*0.25));
            const float rightThreat = threatSound*std::sin(threatPan*static_cast<float>(kTau*0.25));
            streamBuffer[static_cast<std::size_t>(i*2)] = static_cast<std::int16_t>(std::clamp(common + stereo + leftThreat, -0.3f, 0.3f)*32767.0f);
            streamBuffer[static_cast<std::size_t>(i*2 + 1)] = static_cast<std::int16_t>(std::clamp(common - stereo + rightThreat, -0.3f, 0.3f)*32767.0f);
            threatPhase += (71.0 + threatProximity*9.0)/kSampleRate;
            if (threatPhase >= 1.0) threatPhase -= 1.0;
            audioTime += 1.0/kSampleRate;
        }
        UpdateAudioStream(drone, streamBuffer.data(), kStreamFrames);
    }
};

Audio::Audio() : impl_(std::make_unique<Impl>()) {}
Audio::~Audio() { shutdown(); }

bool Audio::init() {
    if (impl_->ready) return true;
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        impl_->ownsDevice = IsAudioDeviceReady();
    }
    if (!IsAudioDeviceReady()) return false;
    SetMasterVolume(0.55f);
    SetAudioStreamBufferSizeDefault(kStreamFrames);
    impl_->drone = LoadAudioStream(kSampleRate, 16, 2);
    if (!IsAudioStreamValid(impl_->drone)) {
        shutdown();
        return false;
    }
    for (int i = 0; i < 3; ++i) impl_->shots[static_cast<std::size_t>(i)] = synthesize(Effect::Shot, i);
    for (int i = 0; i < 4; ++i) impl_->steps[static_cast<std::size_t>(i)] = synthesize(Effect::Step, i);
    impl_->reloadSound = synthesize(Effect::Reload);
    impl_->pickupSound = synthesize(Effect::Pickup);
    impl_->hurtSound = synthesize(Effect::Hurt);
    impl_->whisperSound = synthesize(Effect::Whisper);
    impl_->ready = true;
    impl_->fillDrone();
    impl_->fillDrone();
    PlayAudioStream(impl_->drone);
    return true;
}

void Audio::update(float dt, float tension, bool moving, bool sprinting, bool flashlightOn) {
    if (!impl_->ready || !IsAudioDeviceReady()) return;
    impl_->targetTension = std::clamp(tension, 0.0f, 1.0f);
    impl_->targetLightHum = flashlightOn ? 1.0f : 0.0f;
    // Two sub-buffers keep this bounded even if the device was suspended.
    for (int i = 0; i < 2 && IsAudioStreamProcessed(impl_->drone); ++i) impl_->fillDrone();

    if (moving) {
        impl_->stepClock -= std::clamp(dt, 0.0f, 0.1f);
        if (impl_->stepClock <= 0.0f) {
            const int index = impl_->stepIndex++%4;
            play(impl_->steps[static_cast<std::size_t>(index)], sprinting ? 0.85f : 0.56f,
                 sprinting ? 1.11f : 0.98f, index%2 == 0 ? 0.42f : 0.58f);
            impl_->stepClock = sprinting ? 0.30f : 0.47f;
        }
    } else {
        impl_->stepClock = std::min(impl_->stepClock, 0.12f);
    }
}

void Audio::threat(float pan, float proximity) {
    impl_->targetThreatPan = std::isfinite(pan) ? std::clamp(pan, 0.0f, 1.0f) : 0.5f;
    impl_->targetThreatProximity = std::isfinite(proximity) ? std::clamp(proximity, 0.0f, 1.0f) : 0.0f;
}

void Audio::shoot() {
    if (impl_->ready) play(impl_->shots[static_cast<std::size_t>(impl_->shotIndex++%3)], 0.85f);
}

void Audio::reload() { if (impl_->ready) play(impl_->reloadSound, 0.75f); }
void Audio::pickup() { if (impl_->ready) play(impl_->pickupSound, 0.80f); }
void Audio::hurt() { if (impl_->ready) play(impl_->hurtSound, 0.90f); }
void Audio::whisper() {
    if (impl_->ready && !IsSoundPlaying(impl_->whisperSound)) {
        play(impl_->whisperSound, 0.70f, 0.96f, impl_->stepIndex%2 == 0 ? 0.24f : 0.76f);
    }
}

void Audio::shutdown() {
    if (!impl_) return;
    if (IsAudioDeviceReady()) {
        for (Sound& sound : impl_->shots) { if (IsSoundValid(sound)) UnloadSound(sound); sound = {}; }
        for (Sound& sound : impl_->steps) { if (IsSoundValid(sound)) UnloadSound(sound); sound = {}; }
        for (Sound* sound : {&impl_->reloadSound, &impl_->pickupSound, &impl_->hurtSound, &impl_->whisperSound}) {
            if (IsSoundValid(*sound)) UnloadSound(*sound);
            *sound = {};
        }
        if (IsAudioStreamValid(impl_->drone)) UnloadAudioStream(impl_->drone);
        if (impl_->ownsDevice) CloseAudioDevice();
    }
    impl_->drone = {};
    impl_->ready = false;
    impl_->ownsDevice = false;
    impl_->targetThreatProximity = 0.0f;
    impl_->threatProximity = 0.0f;
}

} // namespace nf
