// S3 GLIDER BOOM — the glider has one job: deliver the drive to the boom.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace gboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(time_); }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return float(x_); }
    float alt() const { return float(h_); }
    // 0 title, 1 carrying the drive, 2 the drive is falling, 3 settling on the boom, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };
    enum class Phase { Carry, Fall, Settle };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(double& nose, double& spoil, bool& release) const;
    void physics(double nose, double spoil, bool release);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void sky();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, bool flip = false,
                    int fog = 0);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    void blit(const Spr& s, float wx, float wy, float scale, int pal, bool flip = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Carry;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool gliderDown_ = false;
    bool wet_ = false;
    int puffN_ = 0;
    int chime_ = -1;
    double time_ = 0;
    double x_ = 0, h_ = 0, v_ = 0, vy_ = 0, att_ = 0;
    double nose_ = 0, spoil_ = 0;
    double dx_ = 0, dy_ = 0, dvx_ = 0, dvy_ = 0;
    double hold_ = 0;
    double camX_ = 0, camH_ = 0, camS_ = 4;
    float chimeT_ = 0, beep_ = 0;
    const char* why_ = "";
    const char* banner_ = "";
    Puff puffs_[8]{};
};

}  // namespace gboom
