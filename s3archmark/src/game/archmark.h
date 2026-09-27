// S3 ARCHMARK — one short end.
// The coin is the mark. Seat it on the gold, stick one arrow in that gold,
// then lift the coin. That finished mark ends the cartridge.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace archmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ARCHMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool onGold() const { return onGold_; }
    bool marked() const { return marked_; }
    int arrows() const { return arrows_; }

private:
    enum class Mode { Title, Place, Aim, Flight, Show, Lift, Pause, Over };

    struct Input {
        float x = 0;
        float y = 0;
        bool action = false;
        bool held = false;
        bool start = false;
        bool back = false;
    };

    struct Pin {
        float x = 0;
        float y = 0;
        bool gold = false;
    };

    void reset();
    void begin();
    void toTitle();
    void loose();
    void stick();
    void finishMark();
    void fail();
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    float swayX(float t) const;
    float swayY(float t) const;
    void blip(float freq);
    void chord(float a, float b, float c);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
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
    bool lifted_ = false;
    bool onGold_ = false;
    bool marked_ = false;
    bool drawing_ = false;
    int arrows_ = 0;
    int drawTick_ = 0;
    int steady_ = 0;
    int flight_ = 0;
    int show_ = 0;
    int pinN_ = 0;
    float clock_ = 0;
    float beep_ = 0;
    float aimX_ = 0;
    float aimY_ = 0;
    float coinX_ = 0;
    float coinY_ = 0;
    float handX_ = 0;
    float handY_ = 0;
    float hitX_ = 0;
    float hitY_ = 0;
    Pin pins_[3]{};
};

}  // namespace archmark
