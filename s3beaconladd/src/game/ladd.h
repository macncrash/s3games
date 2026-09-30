// S3 BEACON LADD — one beacon. Reach the far ladder. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace beaconladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEACON LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title or still by the beacon, 1 off the near shore, 2 at the ladder, 3 done
    int marker() const;
    float heroX() const { return px_; }
    float heroY() const { return py_; }

private:
    enum class Mode { Title, Play, Done };

    struct Plat {
        float x, y, w;
    };

    void beginRun();
    void tickPlay();
    void botIntent(float& mx, bool& jump, bool& up);
    bool onFloor(float x, float y, float* top) const;
    bool hitLadder(float x, float y) const;
    void drawWorld();
    void blit(const gs::Image& img, float x, float y, int pal, bool flip = false);
    void blitM(const gs::Mipped& m, float x, float y, float h, int pal, bool flip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool faceR_ = true;
    bool climbing_ = false;
    bool grounded_ = false;
    int titleT_ = 0;
    int anim_ = 0;
    int stepSnd_ = 0;
    float px_ = 28;
    float py_ = 180;
    float vx_ = 0;
    float vy_ = 0;
    static constexpr int NPLAT = 4;
    Plat plat_[NPLAT] = {};
};

}  // namespace beaconladd
