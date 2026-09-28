// S3 SCULL PASS — row the scull through the gorge.
// Clear the pass before the storm clock. Missing the end fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace scullpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCULL PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float stormLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return hdg_; }
    float speed() const { return speed_; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    void begin();
    void step();
    void pilot();
    void drive(float dt);
    void stroke();
    bool wallHit() const;
    void fail(const char* why);
    void win();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float bowY() const;
    float sternY() const;
    float sx(float wx) const;
    float sy(float wy) const;
    int yawOf(float h) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float playT_ = 0;
    float stormY_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0;
    float speed_ = 0, yawV_ = 0;
    float steer_ = 0;
    float camX_ = 0, camY_ = 0;
    int strokeCd_ = 0;
    int catch_ = 0;
    bool wantStroke_ = false;
    bool wasStroke_ = false;
};

}  // namespace scullpass
