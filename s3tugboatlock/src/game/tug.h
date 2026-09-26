// S3 TUGBOAT LOCK — take the tug through the lock. A scraped gate fails the pass.
// The clock is the other crew. When it runs out, they have the lock.
#pragma once
#include "art.h"
#include "console/system.h"

namespace tuglock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT LOCK"; }
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
    float lowerOpen() const { return openLo_; }
    float upperOpen() const { return openHi_; }
    int phase() const { return int(phase_); }
    // 0 title, 1 the wait, 2 the lower throat, 3 the pound rising, 4 the way out, 5 finished
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Win, Fail };
    enum class Phase { Wait, Throat, Settle, Lift, Out };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void resetPose();
    void begin();
    void showTitle();
    void snapCamera();
    void followCamera();
    void step();
    void controls();
    void pilot(float& thrust, float& steer) const;
    void integrate(float dt);
    void confine();
    void separateRival();
    const char* gateHit() const;
    float nearestLeaf() const;
    bool throatClear(bool upper) const;
    bool inChamber() const;
    void fail(const char* why);
    void win();
    void chime(bool big);
    void horn(float seconds);
    void audio();
    void draw();
    void backdrop();
    void drawWorld();
    void drawGates();
    void drawLeaf(float x0, float y0, float x1, float y1);
    void drawBoat(float wx, float wy, float hdg, int pal, float bob);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool flip = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudR(int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void leafGeom(int which, float open, float& hx, float& hy, float& tx, float& ty) const;
    float halfAt(float y) const;
    float bowY() const;
    float sternY() const;
    float sx(float wx) const;
    float sy(float wy) const;
    int yawFrameOf(float h) const;
    const char* hint() const;
    const char* gateLabel() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Wait;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool commitLo_ = false;
    bool commitHi_ = false;
    const char* why_ = "";
    float t_ = 0;
    float playT_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0;
    float speed_ = 0, yawV_ = 0;
    float thrust_ = 0, steer_ = 0, throttle_ = 0;
    float openLo_ = 0, openHi_ = 0;
    float tgtLo_ = 0, tgtHi_ = 0;
    float fill_ = 0, dwell_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 4.f;
    float shake_ = 0, shx_ = 0, shy_ = 0;
    float wakeT_ = 0, smokeT_ = 0;
    float hornT_ = 0, rubT_ = 0;
    int wakeN_ = 0, smokeN_ = 0;
    int chimeStep_ = -1;
    float chimeT_ = 0;
    bool chimeBig_ = false;
    Puff wakes_[12]{};
    Puff smokes_[8]{};
};

}  // namespace tuglock
