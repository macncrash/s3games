// S3 HARBOR LADD — at the harbor, reach the far ladder. Miss it and the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace harborladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the fairway, 2 at the far ladder, 3 the watch holds, 4 watch over
    int marker() const;
    float heroX() const { return px_; }
    float heroY() const { return py_; }

private:
    enum class Mode { Title, Play, Held, Over };

    struct Plat {
        float x, y, w;
    };

    void beginRun();
    void tickPlay();
    void botIntent(float& mx, bool& jump, bool& up);
    bool onFloor(float x, float y, float* top) const;
    bool hitLadder(float x, float y) const;
    void miss();
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
    float px_ = 36;
    float py_ = 176;
    float vx_ = 0;
    float vy_ = 0;
    float cam_ = 0;
    static constexpr int NPLAT = 4;
    Plat plat_[NPLAT] = {};
};

}  // namespace harborladd
