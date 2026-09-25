// S3 HELI — three pads. Land. Do not hover the clock out.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace heli {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELI"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float left() const { return left_; }
    int pads() const { return got_; }
    int lives() const { return lives_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float vx() const { return vx_; }
    float vy() const { return vy_; }
    int onPad() const { return grounded_ ? on_ : -1; }
    const char* why() const { return why_ && why_[0] ? why_ : "hung"; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    struct Pad {
        const char* name;
        float x, y, half;
        bool done;
    };

    void begin();
    int goal() const;
    float cruiseY(int g) const;
    bool slot(int i, float x) const;
    void pilot(float& col, float& cyc);
    void human(float& col, float& cyc);
    void physics(float dt, float col, float cyc);
    bool faceHit(int i) const;
    void groundOn(int i);
    void crash();
    void win();
    void clockOut();
    void ambience(float dt);
    void camera(float dt);
    void draw();
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void local(float px, float py, float ox, float oy, float& wx, float& wy) const;
    void place(const gs::Mipped& m, float wx, float wy, float worldW, float worldH, int pal, bool flip, float ax, float ay,
               int fog = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawClock();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Pad pads_[3]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = false;
    bool hovering_ = false;
    const char* why_ = "";
    int got_ = 0;
    int lives_ = 3;
    int on_ = -1;
    int face_ = 1;
    float t_ = 0;
    float left_ = 0;
    float col_ = 0.5f;
    float cyc_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float settle_ = 0;
    float invuln_ = 0;
    float shake_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1;
    float popT_ = 0;
    float blipT_ = 0, blipF_ = 0;
    float fanT_ = -1;
    int fanStep_ = -1;
    float rotorMute_ = 0;
    int beepSec_ = -1;
    char pop_[24] = {};
};

}  // namespace heli
