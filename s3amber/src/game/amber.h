// S3 AMBER — one intersection. Stop the ones who run the lamp. Spare the ones who don't.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace amber {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 AMBER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int spared() const { return spared_; }
    int score() const { return score_; }
    int lives() const { return lives_; }
    // 0 title, 1 the shift, 2 the box is clear, 3 the light wins
    int marker() const;

private:
    enum class Mode { Title, Shift, Pause, Won, Lost };
    enum class Result { None, Stopped, SpareStop, SpareGo, Miss, FalseStop };

    struct Car {
        bool alive = false;
        bool runner = false;
        bool released = false;
        bool owed = false;
        bool halted = false;
        bool good = false;
        bool resolved = false;
        int arm = 0;
        double d = 0;
        double speed = 0;
        double held = 0;
        Result result = Result::None;
    };

    void startShift();
    void toTitle();
    void tickShift(bool pressed);
    void stepCar(bool pressed);
    void applyHalt();
    void resolve(Result r);
    void note(const char* s, int pal);
    void blip(float freq, float vol);
    void quiet();
    void fanfare();
    void puffAt(float x, float y);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    int clock() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Car car_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fanOn_ = false;
    int lives_ = 3;
    int score_ = 0;
    int stopped_ = 0;
    int spared_ = 0;
    int next_ = 0;
    int shiftFrame_ = 0;
    int fanStep_ = -1;
    int puffi_ = 0;
    float anim_ = 0;
    float titleT_ = 0;
    float bannerT_ = 0;
    float beep_ = 0;
    float noteT_ = 0;
    float shake_ = 0;
    float pose_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    char nsPh_ = 'G';
    char ewPh_ = 'R';
    const char* note_ = "";
    int notePal_ = PAL_HUD;
    struct Puff {
        float x = 0, y = 0, t = 0;
    } puffs_[8];
};

}  // namespace amber
