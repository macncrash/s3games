// S3 EIGHTMARK — one table. The coin is the mark.
// Set it on the eight and call the pocket. That pocket opens the mark when the
// eight falls there and the cue stays up. Lifting the coin finishes the mark.
// That finished mark ends the cartridge. The rest of the rack is not the job.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace eightmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EIGHTMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool lifted() const { return lifted_; }
    bool opened() const { return opened_; }
    bool rolling() const { return mode_ == Mode::Roll; }
    int shots() const { return shots_; }
    const char* pocketName() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Mark, Aim, Place, Roll, Open, Lift, Pause, Win, Lose };

    struct Body {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool down = false;
        bool fell = false;
        int pocket = -1;
    };
    struct Shot {
        bool ok = false;
        float aim = 0, power = 0.42f;
        int pocket = 1;
    };
    struct Input {
        bool action = false, start = false, back = false, call = false;
        float x = 0, y = 0;
    };

    void place();
    void begin();
    void toTitle();
    void tryMark();
    void shoot();
    void resolve();
    void openMark(int pocket);
    void beginLift();
    void tryLift();
    void finish();
    void fail(const char* why);
    void unmark();
    void dropCue(float x, float y);
    void respotEight();
    void follow();
    void coinOnLip(int pocket);
    bool inCloth(float x, float y) const;
    bool freePoint(float x, float y, int ignore) const;
    bool pathClear(float x0, float y0, float x1, float y1, int ignoreA, int ignoreB) const;
    Shot solve() const;
    bool playOut(Body* b, float aim, float power, int want) const;
    static void stepBodies(Body* b, float dt, bool* clack);
    static void absorb(Body* b);
    static bool moving(const Body* b, float v);

    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void say(const char* s, float time);
    void fanfare();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void backdrop();
    void aimAid();
    void cueDraw();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Body ball_[kBalls]{};
    float coinX_ = kCoinX, coinY_ = kCoinY;
    float handX_ = kHandX, handY_ = kHandY;
    float aim_ = -1.5707963f;
    float power_ = 0.42f;
    float t_ = 0, sayT_ = 0, rollT_ = 0, openT_ = 0, beep_ = 0, fanT_ = 0;
    const char* say_ = "";
    const char* reason_ = "OPEN";
    int called_ = 1;
    int eightPocket_ = 1;
    int shots_ = 0;
    int fanStep_ = -1;
    bool bot_ = false;
    bool over_ = false, won_ = false;
    bool marked_ = false, opened_ = false, lifted_ = false;
    bool onBall_ = false, solved_ = false, pocketSnd_ = false;
};

}  // namespace eightmark
