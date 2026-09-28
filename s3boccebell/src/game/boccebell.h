// S3 BOCCE BELL — one end, three bowls.
// The pallino sits inside a brass bell painted on the clay.
// A bowl rings it only if it dies inside that circle, still on the court.
// Short of the foul line, into a side board, or onto the back board: the try dies.
// Outside the bell the try dies too. Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace boccebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOCCE BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Roll, Dead, Ring, Leave, Over };

    struct Bowl {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool live = false;
        bool moving = false;
        bool burned = false;
    };

    void toTitle();
    void begin();
    void launch();
    void coast(float dt);
    void separate();
    void settle();
    void ring();
    void dieTry(const char* why);
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Bowl bowl_[3]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int n_ = 0;
    int hold_ = 0;
    const char* why_ = "";
    float aim_ = 0.f;
    float power_ = 0.45f;
    float powerDir_ = 1.f;
    float clock_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.12f;
};

}  // namespace boccebell
