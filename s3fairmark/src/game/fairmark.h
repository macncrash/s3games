// S3 FAIRMARK — one ring booth. The gold coin is the mark.
// Set it on the gold bottle. A ring that drops on that bottle opens the mark.
// Lifting the coin finishes it. That finished mark ends the cartridge.
// The rest of the midway is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fairmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIRMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool lifted() const { return lifted_; }
    bool opened() const { return opened_; }
    bool finished() const { return finished_; }
    int rings() const { return thrown_; }
    const char* phase() const;

private:
    enum class Mode { Title, Mark, Aim, Flight, Open, Lift, Pause, Over };

    struct Input {
        bool action = false, start = false, back = false;
        float x = 0, y = 0;
    };

    void place();
    void begin();
    void toTitle();
    void tryMark();
    void release();
    void resolve();
    void beginLift();
    void tryLift();
    void fail();
    float hangX() const;
    int bottleAt(float x) const;

    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void say(const char* s, float time);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false,
             bool feet = false, int fog = 0);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Mark;
    float coinX_ = 0, coinY_ = 0;
    float handX_ = 0, handY_ = 0;
    float swingT_ = 0, nudge_ = 0;
    float landX_ = 0, flight_ = 0, openT_ = 0;
    float clock_ = 0, sayT_ = 0, beep_ = 0;
    const char* say_ = "";
    int thrown_ = 0;
    int landBottle_ = -1;
    bool bot_ = false;
    bool over_ = false, won_ = false;
    bool marked_ = false, opened_ = false, lifted_ = false, finished_ = false;
    bool onGold_ = false, wasSweet_ = false;
};

}  // namespace fairmark
