// S3 SLED LOCK — one toboggan, one frozen lock. Pass it. A scrape fails the run.
#pragma once
#include "art.h"
#include "console/system.h"

namespace sledlock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return playT_; }
    int phase() const { return int(phase_); }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return hdg_; }
    float speed() const { return speed_; }
    float slide() const;
    float lowerOpen() const { return openLo_; }
    float upperOpen() const { return openHi_; }
    // 0 title, 1 the glide, 2 the lower throat, 3 the ice heaving, 4 the way out, 5 finished
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Win, Fail };
    enum class Phase { Glide, Throat, Shut, Rise, Out };

    struct Puff {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0, sc = 1;
    };
    struct Flake {
        float x = 0, y = 0, sp = 20, sc = 2;
    };

    void resetPose();
    void seedFlakes();
    void begin();
    void showTitle();
    void snapCamera();
    void followCamera();
    void step();
    void controls();
    void pilot(float& kick, float& brake, float& lean, bool& hail) const;
    void integrate(float dt);
    void confine();
    const char* gateHit() const;
    bool throatClear(bool upper) const;
    bool inChamber() const;
    bool lined() const;
    void fail(const char* why);
    void win();
    void chime(bool big);
    void audio();
    void flakes(float dt);
    void puffs(float dt);
    void draw();
    void backdrop();
    void drawWorld();
    void drawGates();
    void drawSled();
    void drawLeaf(float x0, float y0, float x1, float y1);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool flip = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void leafGeom(int which, float open, float& hx, float& hy, float& tx, float& ty) const;
    float halfAt(float y) const;
    float slopeAt() const;
    float bowY() const;
    float sternY() const;
    float sx(float wx) const;
    float sy(float wy) const;
    float viewTop() const;
    float viewBot() const;
    int yawFrame() const;
    const char* hint() const;
    const char* gateLabel() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Glide;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool commitLo_ = false;
    bool commitHi_ = false;
    const char* why_ = "";
    float t_ = 0;
    float playT_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0;
    float speed_ = 0, fall_ = 0, lat_ = 0, yawV_ = 0;
    float kick_ = 0, brake_ = 0, lean_ = 0;
    float openLo_ = 0, openHi_ = 0;
    float tgtLo_ = 0, tgtHi_ = 0;
    float fill_ = 0, dwell_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 8.f;
    float shake_ = 0, shx_ = 0, shy_ = 0;
    float puffT_ = 0, ayeT_ = 0;
    float chimeT_ = 0;
    int chimeStep_ = -1;
    int puffN_ = 0;
    bool chimeBig_ = false;
    bool deny_ = false;
    Puff puffs_[16]{};
    Flake flakes_[18]{};
};

}  // namespace sledlock
