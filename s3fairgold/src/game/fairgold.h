// S3 FAIR GOLD — a short midway ring booth, then the gate.
// Cream bottles count one. Gold bottles count two. Three rings.
// The fare is 4, so cream alone cannot pay it. Leave when the double does.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fairgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIR GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return gold_ * 2 + cream_; }
    int rings() const { return thrown_; }
    int fare() const { return 4; }
    const char* phase() const;

private:
    enum class Mode { Title, Aim, Flight, Show, Walk, End, Pause };

    struct Bottle {
        float x = 0;
        bool gold = false;
        bool rung = false;
    };
    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
        float x = 0;
    };

    void place();
    void begin();
    void toTitle();
    void release();
    void resolve();
    void startWalk();
    void finishWalk();
    bool canLeave() const;
    float ringX() const;
    int bottleAt(float x) const;
    int botTarget() const;
    Mode view() const;

    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void hush();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop(bool gate);
    void draw();
    void drawBooth();
    void drawGate();
    void drawHud();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Bottle bottle_[5]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool wasHot_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int thrown_ = 0;
    int last_ = 0;
    float swingT_ = 0;
    float nudge_ = 0;
    float clock_ = 0;
    float flight_ = 0;
    float showT_ = 0;
    float beep_ = 0;
    float landX_ = 160.f;
    float fromX_ = 160.f;
    float walker_ = 24.f;
    const char* say_ = "";
};

}  // namespace fairgold
