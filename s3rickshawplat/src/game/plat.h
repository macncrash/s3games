// S3 RICKSHAW PLAT — stop the step level with the platform.
// Pedal to the porch. Raise or lower the floor. Hold it still on the mark.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace rickplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float step() const { return step_; }
    float speed() const { return v_; }
    // 0 title, 1 the street, 2 beside the porch, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(float& pedal, float& lift) const;
    void physics(float pedal, float lift);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
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
    bool beside_ = false;
    bool snapCam_ = true;
    const char* why_ = "";
    const char* banner_ = "";
    float legT_ = 0, anim_ = 0, idle_ = 0;
    float x_ = 0, step_ = 0, v_ = 0, lift_ = 0, liftV_ = 0, pedal_ = 0, hold_ = 0;
    float camX_ = 0, camH_ = 0, camS_ = 4.f;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    int puffN_ = 0;
    Puff puffs_[6]{};
};

}  // namespace rickplat
