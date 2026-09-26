// S3 EIGHTBELL — three strokes at the eight.
// The bell hangs on the far pocket. It rings only when the 8 falls there
// and the cue stays on the cloth. A miss, a jaw, a wide pocket, a short
// stroke, or a scratch: that try dies. The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace eightbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EIGHTBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool rolling() const { return mode_ == Mode::Roll; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Roll, Dead, Ring, Leave, Pause, Over };
    enum class Fate { Ring, Scratch, Wide, Jaw, Short, Miss };

    struct Body {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool down = false;
        bool fell = false;
        int pocket = -1;
    };
    struct Stroke {
        bool ok = false;
        float aim = 0;
        float power = 0.46f;
    };
    struct Input {
        bool action = false, start = false, back = false, fine = false;
        float x = 0, y = 0;
    };

    void spot();
    void fresh();
    void toTitle();
    void begin();
    void shoot();
    void resolve();
    void ring();
    void dieTry(Fate why);
    bool prove();
    bool layout() const;
    Stroke solve() const;
    Fate rollCopy(Body* b, float aim, float power) const;
    Fate fateOf(const Body* b) const;
    bool inCloth(float x, float y) const;
    bool pathClear(float x0, float y0, float x1, float y1, int ignoreA, int ignoreB) const;
    static void step(Body* b, float dt, bool* clack);
    static void absorb(Body* b);
    static bool moving(const Body* b, float v);

    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void steer(float x, float y, bool fine);
    static const char* fateName(Fate f);
    void blip(float freq, float vol, float hold);
    void tickAudio(float dt);
    void fanfare();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void aimAid();
    void cueDraw();
    void lamps();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Fate whyFate_ = Fate::Miss;
    Body ball_[kBalls]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool solved_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int shots_ = 0;
    int sunkSnd_ = 0;
    int fanStep_ = -1;
    float aim_ = -1.5707963f;
    float power_ = 0.46f;
    float solvedAim_ = -1.5707963f;
    float solvedPower_ = 0.46f;
    float t_ = 0;
    float ready_ = 0;
    float rollT_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.16f;
    float bellTick_ = 0;
    float chimeHold_ = 0;
    float toneT_ = 0;
    float fanT_ = 0;
    const char* why_ = "OPEN";
};

}  // namespace eightbell
