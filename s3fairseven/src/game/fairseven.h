// S3 FAIR SEVEN — one ring apiece at the bottle booth.
// Cream counts 1, gold counts 2, the bell bottle counts 3.
// First tally to reach 7 wins. A 6 is still short, and the match goes on.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fairseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIR SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool shortSix() const { return shortSix_; }
    int you() const { return you_; }
    int them() const { return them_; }
    const char* phase() const;

private:
    enum class Mode { Title, Aim, Flight, Show, Win, Lose, Pause };

    struct Toss {
        int pts = 0;
        bool yours = false;
    };
    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
        float x = 0;
    };

    void resetMatch();
    void begin();
    void toTitle();
    void armSwing();
    void release();
    void resolve();
    void win();
    void lose();
    void afterShow();
    bool audit();
    bool ledgerOk() const;
    bool ready() const;
    int aimBottle() const;
    float ringX() const;
    Mode view() const;

    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;

    void blip(float freq);
    void chord(float a, float b, float c);
    void hush();
    void tickAudio(float dt);

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0);
    void word(const gs::Image& img, float cx, float cy, int pal, float mul = 1.f);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int palFor(int pts) const;
    const gs::Image& digit(int pts) const;

    void backdrop();
    void draw();
    void drawBooth();
    void drawHud();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Toss log_[32]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool shortSix_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int gain_ = 0;
    int tossN_ = 0;
    int hot_ = -1;
    int fanStep_ = -1;
    float swingT_ = 0;
    float nudge_ = 0;
    float aimT_ = 0;
    float clock_ = 0;
    float flight_ = 0;
    float showT_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float landX_ = 160.f;
    float fromX_ = 160.f;
    const char* say_ = "";
};

}  // namespace fairseven
