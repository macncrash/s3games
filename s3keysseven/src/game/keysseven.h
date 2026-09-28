// S3 KEYS SEVEN — play the keys until one side is first to seven, then leave.
// A bar is four notes. A clean bar is yours. A slip hands the bar to them.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keysseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYS SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }
    bool playing() const { return mode_ == Mode::Play; }

private:
    enum class Mode { Title, Play, Win, Lose };

    struct Note {
        int lane = 0;
        int bar = 0;
        float y = 0;
        bool gone = false;
        bool hit = false;
    };

    void begin();
    void updatePlay();
    void strike(int lane);
    void settle(int bar);
    void win();
    void lose();
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
    Note notes_[kNotes]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool chartOk_ = true;
    int you_ = 0;
    int them_ = 0;
    int bars_ = 0;
    int clean_ = 0;
    int spawned_ = 0;
    int gap_ = 0;
    int hold_ = 0;
    int hits_ = 0;
    float clock_ = 0;
    float toneT_ = 0;
    float keyLit_[kLanes] = {};
};

}  // namespace keysseven
