// S3 FAIR — three midway booths, then the gate. Leave only with a score.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fair {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return boothScore_[0] + boothScore_[1] + boothScore_[2]; }
    int ring() const { return boothScore_[0]; }
    int dart() const { return boothScore_[1]; }
    int bell() const { return boothScore_[2]; }
    // 0 title, 1 walk, 2 ring, 3 dart, 4 bell, 5 bark, 6 leave, 7 pause
    int marker() const;
    const char* phase() const;

private:
    enum class Mode { Title, Walk, Ring, Dart, Bell, Bark, Leave, Pause };

    struct Bal {
        float home = 0, phase = 0, freq = 1;
        int pts = 0, pal = 0;
        bool live = false;
    };
    struct Pop {
        float x = 0, y = 0, life = 0;
        int pts = 0;
        bool on = false;
    };

    void newGame();
    void toTitle();
    void openBooth(int i);
    void bank();
    void leave();
    void say(const char* s);
    void tryUse();
    float goal() const;

    void updateWalk(bool action, float slide);
    void updateRing(bool action, float slide);
    void updateDart(bool action, float slide);
    void updateBell(bool action);

    float ringBase() const;
    float ringX() const;
    float meter() const;
    int strikeScore(float m) const;
    const char* strikeName(float m) const;
    float balX(int i) const;
    float balY(int i) const;
    int richest() const;
    int atAim(float x) const;
    void popup(float x, float y, int pts);

    void blip(float freq, float vol = 0.08f);
    void chord(float a, float b, float c);
    void missSnd();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal = PAL_HUD);
    float sx(float x) const;

    void draw();
    void drawSky(Mode view);
    void drawMidway(Mode view);
    void drawRing();
    void drawDart();
    void drawBell();
    void drawPops();
    void drawHelp(Mode view);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Walk;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool swung_ = false;
    bool rung_ = false;
    bool wasSweet_ = false;
    int boothScore_[3] = {};
    bool played_[3] = {};
    int booth_ = -1;
    int thrown_ = 0;
    int acc_ = 0;
    int hitId_ = -1;
    int face_ = 1;
    float slide_ = 0;
    float px_ = 96;
    float cam_ = 0;
    float clock_ = 0;
    float playT_ = 0;
    float cool_ = 0;
    float beep_ = 0;
    float flight_ = 0;
    float show_ = 0;
    float landX_ = 160;
    float aim_ = 160;
    float struck_ = 0;
    float nudge_ = 0;
    float shake_ = 0;
    float burstX_ = 0, burstY_ = 0, burstT_ = 0;
    Bal bal_[5];
    Pop pops_[4];
    char bark_[24] = {};
};

}  // namespace fair
