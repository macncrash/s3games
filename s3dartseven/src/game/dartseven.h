// S3 DART SEVEN — one board, two throwers, one dart apiece.
// A treble adds 3, a double 2, a single 1, the outer bull 1, the bull 2.
// First tally to reach 7 wins. Going past 7 still counts.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace dartseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DART SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Aim, Flight, Show, Win, Lose, Pause };

    struct Hit {
        int mul = 0;
        char name[8] = {};
    };
    struct Pin {
        float x = 0, y = 0;
        bool yours = true;
        bool on = false;
    };

    void begin();
    void launch();
    void stick();
    void afterShow();
    void autoAim();
    void moveAim(float mx, float my);
    bool sweet() const;
    bool wantsThrow() const;
    bool audit();
    Hit scoreAt(float x, float y) const;
    void bedPoint(int seg, float rad, float& x, float& y) const;
    uint32_t rnd();

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void dartAt(float x, float y, float h, bool yours);
    void meterRow();
    void lamps();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool yours_ = true;
    bool wasSweet_ = false;
    int you_ = 0;
    int them_ = 0;
    int gain_ = 0;
    int thrown_ = 0;
    int pinN_ = 0;
    char last_[8] = {};
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    float fromX_ = kCx;
    float fromY_ = 0;
    float destX_ = kCx;
    float destY_ = kCy;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float showT_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    int flightT_ = 0;
    uint32_t rng_ = 0x7E11u;
    Pin pin_[8];
};

}  // namespace dartseven
