// S3 MAIL VAN PLAT — stop level with the platform.
// The cargo floor has to sit flush with the dock, and the door has to hold still.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace mailplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAIL VAN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return door_; }
    float deck() const { return deck_; }
    float speed() const { return v_; }
    // 0 title, 1 the street, 2 at the dock, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& thrust, float& lift) const;
    void physics(float thrust, float lift);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void audio();
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float sx(float wx) const;
    float sy(float wy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool atDock_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    float legT_ = 0, anim_ = 0;
    float door_ = 0, deck_ = 0, v_ = 0, lift_ = 0, liftV_ = 0, thrust_ = 0, hold_ = 0, idle_ = 0;
    float camX_ = 0, camH_ = 0, camS_ = 12.f;
    float shake_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace mailplat
