// S3 DARTMARK — one visit at the 20.
// A single is one mark, a double two, a treble three.
// Three marks close 20. That finished mark ends the cartridge.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace dartmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DARTMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    int marks() const { return marks_; }
    int darts() const { return thrown_; }
    const char* bed() const { return out_; }

private:
    enum class Mode { Title, Aim, Flight, Show, Pause, Win, Lose };

    struct Hit {
        int number = 0;
        int mul = 0;
        char name[8] = {};
    };
    struct Pin {
        float x = 0, y = 0;
        bool on = false;
    };

    void begin();
    void launch();
    void stick();
    void finishMark();
    void fail();
    void afterShow();
    bool audit();
    Hit scoreAt(float x, float y) const;
    void bedPoint(int seg, float rad, float& x, float& y) const;
    void moveAim(float mx, float my);
    void botPlay();
    bool sweet() const;
    bool wantsThrow() const;
    uint32_t rnd();

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void dartSpr(float x, float y, float h, int slot);
    void ringAt(float x, float y, float meter, bool cross);
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
    bool finished_ = false;
    bool wasSweet_ = false;
    int marks_ = 0;
    int thrown_ = 0;
    int visitN_ = 0;
    char out_[8] = {};
    char last_[8] = {};
    float aimX_ = kCx, aimY_ = kCy;
    float landX_ = kCx, landY_ = kCy;
    float fromX_ = kCx, fromY_ = 0;
    float destX_ = kCx, destY_ = kCy;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float showT_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    int flightT_ = 0;
    uint32_t rng_ = 0x0D47u;
    Pin pin_[3];
};

}  // namespace dartmark
