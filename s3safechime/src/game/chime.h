// S3 SAFECHIME — Safe: the hour has to chime.
// Three tumblers, each caught from the printed direction. The door stays
// shut until they sit, and the bell only answers on the hour.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace safechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SAFE CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    bool dialing() const { return mode_ == Mode::Dial; }
    int hour() const;
    int minute() const;
    int second() const;
    int tumblers() const { return stage_; }
    int dial() const { return dial_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Dial, Wait, Chime, Fail };

    void begin();
    void nudge(int dir);
    void latch();
    void botDial();
    void humanDial();
    void beginChime();
    void beginFail(const char* why);
    void tone(float freq, float vol);
    void decay();
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void word(const gs::Image& img, float cx, float y, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool frozen_ = false;
    bool moved_ = false;
    int dial_ = 0;
    int stage_ = 0;
    int lastDir_ = 0;
    int spinCool_ = 0;
    int lock_ = 0;
    int hold_ = 0;
    int playFrames_ = 0;
    float clock_ = 0;
    float toneT_ = 0;
    float bellAmp_ = 0;
    const char* reason_ = "hour silent";
};

}  // namespace safechime
