// S3 CLIFF KILO — finish the kilometer on the cliff shelf without touching a wheel.
// Missing the end of the leg fails the run. Leaving the shelf fails it too.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace cliffkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFFKILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 the shelf, 2 among the sheaves, 3 the last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& gas, float& brake, float& steer) const;
    void physics(float gas, float brake, float steer);
    int nextWheel() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void skyShelf();
    bool project(float wz, float wy, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    int chime_ = -1;
    float chimeT_ = 0;
    float beep_ = 0;
    float time_ = 0;
    float s_ = 0;
    float y_ = 0;
    float pan_ = 0;
    float v_ = 0;
    float gas_ = 0, brake_ = 0, steer_ = 0;
    float shake_ = 0;
};

}  // namespace cliffkilo
