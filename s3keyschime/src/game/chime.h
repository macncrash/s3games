// S3 KEYSCHIME — a short keys. The hour has to chime.
// Play the five notes clean. When the clock strikes twelve the bell answers.
// A missed note lets the hour pass in silence.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keyschime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYS CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    bool waiting() const { return mode_ == Mode::Wait || mode_ == Mode::Chime; }
    int hour() const { return frozen_ ? fh_ : liveHour(); }
    int minute() const { return frozen_ ? fm_ : liveMinute(); }
    int second() const { return frozen_ ? fs_ : liveSecond(); }
    int hits() const { return hits_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Wait, Chime, Fail };

    struct Note {
        int lane = 0;
        float y = 0;
        bool gone = false;
        bool hit = false;
    };

    void begin();
    void arm();
    void updatePlay();
    void strike(int lane);
    void miss();
    void strikeHour();
    void passHour();
    void tone(float freq, float vol);
    void decay();
    void splitLive(int& h, int& m, int& s) const;
    int liveHour() const;
    int liveMinute() const;
    int liveSecond() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void word(const gs::Image& img, float cx, float y, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Note notes_[kPhrase]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool silent_ = false;
    bool frozen_ = false;
    int hits_ = 0;
    int spawned_ = 0;
    int gap_ = 0;
    int hold_ = 0;
    int playFrames_ = 0;
    int fh_ = 12, fm_ = 0, fs_ = 0;
    float clock_ = 0;
    float toneT_ = 0;
    float bellAmp_ = 0;
    float keyLit_[kLanes] = {};
    const char* reason_ = "hour silent";
};

}  // namespace keyschime
