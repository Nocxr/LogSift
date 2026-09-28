// Notification audio device and synthesized sound presets.
// Included by main.cpp; keep this module focused on this responsibility.

struct SynthTone { float hz; float start; float duration; float gain; };

SDL_AudioStream* gNotificationAudioStream = nullptr;
std::chrono::steady_clock::time_point gNotificationAudioPlaybackUntil{};

void ShutdownNotificationAudio() {
    if (!gNotificationAudioStream) return;
    SDL_DestroyAudioStream(gNotificationAudioStream);
    gNotificationAudioStream = nullptr;
    gNotificationAudioPlaybackUntil = {};
}

// An open SDL playback stream keeps CoreAudio running even when it is silent.
// Give the device time to play its last buffered samples, then release it.
void MaybeShutdownNotificationAudio() {
    if (!gNotificationAudioStream) return;
    const auto now = std::chrono::steady_clock::now();
    if (now < gNotificationAudioPlaybackUntil + std::chrono::milliseconds(750))
        return;
    if (SDL_GetAudioStreamQueued(gNotificationAudioStream) <= 0)
        ShutdownNotificationAudio();
}


bool EnsureNotificationAudio() {
    if (gNotificationAudioStream) return true;
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = 48000;
    gNotificationAudioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!gNotificationAudioStream) return false;
    if (SDL_ResumeAudioStreamDevice(gNotificationAudioStream)) return true;
    ShutdownNotificationAudio();
    return false;
}

std::vector<float> MakeNotificationPcm(const std::vector<SynthTone>& tones, float totalSeconds) {
    constexpr int rate = 48000;
    const int frames = static_cast<int>(totalSeconds * rate);
    std::vector<float> pcm(static_cast<size_t>(frames), 0.0f);
    constexpr float pi = 3.14159265358979323846f;
    for (const auto& t : tones) {
        const int begin = std::max(0, static_cast<int>(t.start * rate));
        const int end = std::min(frames, static_cast<int>((t.start + t.duration) * rate));
        for (int i=begin;i<end;++i) {
            const float local=(i-begin)/(float)rate;
            const float attack=std::min(1.0f, local/0.018f);
            const float remain=std::max(0.0f,t.duration-local);
            const float release=std::min(1.0f,remain/0.12f);
            const float env=attack*release*std::exp(-2.2f*local/std::max(0.05f,t.duration));
            const float phase=2.0f*pi*t.hz*local;
            pcm[(size_t)i] += t.gain*env*(std::sin(phase)+0.18f*std::sin(phase*2.0f));
        }
    }
    for(auto& s:pcm) s=std::clamp(s,-0.9f,0.9f);
    return pcm;
}

void PlaySynthPreset(int preset, bool startEvent) {
    if (preset<=0) return;
    std::vector<SynthTone> tones; float seconds=0.45f;
    if (startEvent) {
        if (preset==1) { tones={{880,0.00f,0.10f,0.20f}}; seconds=0.16f; } // Tick
        else if (preset==2) { tones={{520,0.00f,0.22f,0.18f},{660,0.07f,0.20f,0.13f}}; seconds=0.34f; } // Soft
        else if (preset==3) { tones={{620,0.00f,0.22f,0.18f},{830,0.10f,0.26f,0.16f}}; seconds=0.42f; } // Chime
        else if (preset==4) { tones={{760,0.00f,0.08f,0.18f},{760,0.13f,0.08f,0.16f}}; seconds=0.28f; } // Pulse
        else if (preset==5) { tones={{420,0.00f,0.28f,0.13f},{760,0.05f,0.30f,0.15f}}; seconds=0.42f; } // Sweep
        else if (preset==6) { tones={{1040,0.00f,0.12f,0.18f},{780,0.10f,0.16f,0.13f}}; seconds=0.30f; } // Ping
        else { tones={{700,0.00f,0.09f,0.16f},{920,0.12f,0.09f,0.17f},{700,0.24f,0.09f,0.14f}}; seconds=0.38f; } // Triple
    } else {
        if (preset==1) { tones={{560,0.00f,0.25f,0.16f},{700,0.08f,0.24f,0.12f}}; seconds=0.38f; } // Soft
        else if (preset==2) { tones={{620,0.00f,0.28f,0.19f},{830,0.12f,0.30f,0.17f}}; seconds=0.50f; } // Chime
        else if (preset==3) { tones={{660,0.00f,0.26f,0.18f},{880,0.12f,0.32f,0.19f},{1100,0.23f,0.30f,0.13f}}; seconds=0.60f; } // Success
        else if (preset==4) { tones={{440,0.00f,0.18f,0.20f},{330,0.17f,0.30f,0.22f}}; seconds=0.52f; } // Attention
        else if (preset==5) { tones={{520,0.00f,0.13f,0.18f},{390,0.12f,0.18f,0.18f},{300,0.27f,0.24f,0.15f}}; seconds=0.58f; } // Offline
        else if (preset==6) { tones={{900,0.00f,0.08f,0.18f},{1120,0.08f,0.10f,0.14f}}; seconds=0.24f; } // Pop
        else if (preset==7) { tones={{740,0.00f,0.10f,0.14f},{980,0.08f,0.12f,0.17f},{1240,0.16f,0.13f,0.12f}}; seconds=0.34f; } // Spark
        else { tones={{360,0.00f,0.30f,0.17f},{270,0.10f,0.34f,0.13f}}; seconds=0.50f; } // Low
    }
    auto pcm = MakeNotificationPcm(tones, seconds);
    if (!EnsureNotificationAudio()) return;

    // Keep the stream while sounds are queued so a new notification does not
    // cut off the previous waveform. The idle loop releases it after playback.
    if (SDL_PutAudioStreamData(gNotificationAudioStream, pcm.data(),
            static_cast<int>(pcm.size() * sizeof(float)))) {
        SDL_FlushAudioStream(gNotificationAudioStream);
        const auto now = std::chrono::steady_clock::now();
        gNotificationAudioPlaybackUntil =
            std::max(now, gNotificationAudioPlaybackUntil) +
            std::chrono::milliseconds(static_cast<int>(seconds * 1000.0f));
    }
}

void PlayEndSound(const Config& cfg, bool failure=false) {
#ifdef _WIN32
    if (!failure && !cfg.toastSoundFile.empty()) {
        PlaySoundA(cfg.toastSoundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        return;
    }
#endif
    PlaySynthPreset(failure ? cfg.failureSoundPreset : cfg.endSoundPreset, false);
}


