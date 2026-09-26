// S3 TUGBOAT PASS — take the tug and clear the pass.
// The storm clock is the other crew. A touch on the wall fails the pass.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tugpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return playT_; }
    float crewLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return hdg_; }
    float speed() const { return speed_; }
    float minGap() const { return minGap_; }
    // 0 title, 1 the pass, 2 the cut, 3 the lee, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    struct Puff {
        float x = 0, y = 0, life = 0, seed = 0;
    };

    void resetPose();
    void begin();
    void showTitle();
    void snapCamera();
    void followCamera();
    void step();
    void ambience();
    void controls();
    void pilot(float& thrust, float& steer) const;
    void integrate(float dt);
    const char* struck() const;
    bool hullClear() const;
    void sampleHull(float* xs, float* ys, int& n) const;
    void fail(const char* why);
    void win();
    void chime();
    void horn(float seconds);
    void audio();
    void draw();
    void backdrop();
    void drawWorld();
    void drawStorm();
    void drawBoat(float wx, float wy, float hdg, int pal, float bob, int fog);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool flip = false,
             int fog = 0);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudR(int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    float centerAt(float y) const;
    float halfAt(float y) const;
    float currentAt(float y) const;
    float wallGap(float x, float y) const;
    float crewMax() const;
    float urgency() const;
    float rivalY() const;
    float rivalX() const;
    float rivalHeading() const;
    float sx(float wx) const;
    float sy(float wy) const;
    int yawFrameOf(float h) const;
    int fogAt(float wy) const;
    const char* hint() const;
    const char* telegraph() const;
    bool flashOn() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float t_ = 0;
    float playT_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0;
    float speed_ = 0, yawV_ = 0, driftX_ = 0;
    float thrust_ = 0, steer_ = 0, throttle_ = 0;
    float camX_ = 0, camY_ = 20, zoom_ = 5.f;
    float shake_ = 0, shx_ = 0, shy_ = 0;
    float wakeT_ = 0, smokeT_ = 0, hornT_ = 0;
    float minGap_ = 99.f;
    float beepHold_ = 0;
    int beepSec_ = -1;
    int wakeN_ = 0, smokeN_ = 0;
    int chimeStep_ = -1;
    float chimeT_ = 0;
    bool flashWas_ = false;
    Puff wakes_[12]{};
    Puff smokes_[10]{};
};

}  // namespace tugpass
