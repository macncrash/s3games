// S3 LOCK — one canal lock. Steer the narrowboat through. Don't hit the gates.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace s3lock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return playT_; }
    int phase() const { return int(phase_); }
    float speed() const { return speed_; }
    float lateral() const { return x_; }
    float heading() const { return hdg_; }
    float lowerOpen() const { return openLo_; }
    float upperOpen() const { return openHi_; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };
    enum class Phase { Approach, Lower, Chamber, Fill, Upper };

    void reset();
    void begin();
    void step();
    void pilot(float& thrust, float& rudder) const;
    void confine();
    const char* gateHit();
    void fail(const char* why);
    void win();
    void bell(bool big);
    void audio();
    void draw();
    void backdrop();
    void drawWorld();
    void drawBoat();
    void drawGates();
    void drawBar(float ax, float ay, float bx, float by);
    void leafGeom(int which, float open, float& px, float& py, float& tx, float& ty) const;
    void blit(const gs::Mipped& m, float cx, float cy, float dw, float dh, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    float sx(float wx) const;
    float sy(float wy) const;
    float halfAt(float y) const;
    float windAt(float y) const;
    float bowY() const;
    float sternY() const;
    const char* hint() const;
    int yawFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float t_ = 0;
    float playT_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0;
    float speed_ = 0, latV_ = 0, yawV_ = 0;
    float thrust_ = 0, rudder_ = 0;
    float openLo_ = 0, openHi_ = 0;
    float tgtLo_ = 0, tgtHi_ = 0;
    float dwell_ = 0, hold_ = 0, fill_ = 0;
    float shake_ = 0, shx_ = 0, shy_ = 0;
    int bellStep_ = -1;
    float bellT_ = 0;
    bool bellBig_ = false;
    bool called_ = false;
};

}  // namespace s3lock
