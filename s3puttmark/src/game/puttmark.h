// S3 PUTTMARK — one green. The coin is the mark.
// A holed putt opens it. Lifting the coin finishes it, and that ends the cartridge.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace puttmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PUTTMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool lifted() const { return lifted_; }
    bool holed() const { return holed_; }
    bool rolling() const { return mode_ == Mode::Roll; }
    int strokes() const { return strokes_; }

private:
    enum class Mode { Title, Mark, Hold, Aim, Roll, Hole, Lift, Pause, Over };
    enum class Stop { Moving, Rest, Sunk, Off };

    struct Lie {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool rest = true, sunk = false, off = false;
    };
    struct Input {
        bool action = false, start = false, back = false;
        float x = 0, y = 0;
    };

    void place();
    void begin();
    void tryMark();
    void putt(float ang, float spd);
    void openHole();
    void beginLift();
    void tryLift();
    void finish();
    void fail();
    void resolve(Stop s);
    Stop roll(Lie& b, float dt) const;
    bool sinks(float x, float y, float ang, float spd) const;
    bool solve(float x, float y, float& ang, float& spd) const;
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void say(const char* s, float time);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false,
             bool feet = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Mark;
    Lie ball_{};
    float coinX_ = 0, coinY_ = 0;
    float handX_ = 0, handY_ = 0;
    float aim_ = 0, spd_ = 0;
    float meter_ = 0, meterDir_ = 1;
    float clock_ = 0, holdT_ = 0, holeT_ = 0, rollT_ = 0, sayT_ = 0, beep_ = 0;
    const char* say_ = "";
    int strokes_ = 0;
    bool bot_ = false;
    bool over_ = false, won_ = false;
    bool marked_ = false, holed_ = false, lifted_ = false;
    bool solved_ = false, charging_ = false, onBall_ = false;
};

}  // namespace puttmark
