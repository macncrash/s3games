// S3 KARTBOX — stop the kart inside the painted box.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace kartbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTBOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float speed() const { return v_; }
    // 0 title, 1 the run, 2 inside the box, 3 holding still, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& gas, float& brake) const;
    void physics(float gas, float brake);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inside_ = false;
    bool snapCam_ = true;
    const char* why_ = "";
    const char* banner_ = "";
    float legT_ = 0, anim_ = 0;
    float x_ = 0, v_ = 0;
    float gas_ = 0, brake_ = 0, hold_ = 0, idle_ = 0, roll_ = 0;
    float camX_ = 0, camS_ = 12.f;
    float shake_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace kartbox
