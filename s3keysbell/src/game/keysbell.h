// S3 KEYS BELL — play the phrase. The bell rings only on a clean try.
// A missed note kills that try. The third dead try ends it with the bell silent.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keysbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYS BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool rung() const { return rung_; }
    bool playing() const { return mode_ == Mode::Play; }
    int tryNo() const { return tryNo_; }
    int deadTries() const { return dead_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Gap, Ring, Leave, Dead };

    struct Note {
        int lane = 0;
        float y = 0;
        bool gone = false;
        bool hit = false;
    };

    void begin();
    void armPhrase();
    void updatePlay();
    void strike(int lane);
    void killTry();
    void ring();
    void tone(float freq, float vol);
    void decay();
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
    bool rules_ = false;
    bool rung_ = false;
    bool chartOk_ = true;
    int tryNo_ = 0;
    int dead_ = 0;
    int spawned_ = 0;
    int hits_ = 0;
    int gap_ = 0;
    int hold_ = 0;
    float clock_ = 0;
    float toneT_ = 0;
    float bellAmp_ = 0.2f;
    float keyLit_[kLanes] = {};
    const char* reason_ = "bell silent";
};

}  // namespace keysbell
