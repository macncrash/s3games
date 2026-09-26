// S3 SLED PLAT — one freight sled, one station platform.
// Stop the cargo door in the bay, runners on the timber, nose level.
// Close is not level. Stopping short, tipped, or past the end fails the run.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace sledplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float bed() const { return bed_; }
    float pitch() const { return att_; }
    float speed() const { return v_; }
    // 0 title, 1 the grade, 2 on the platform, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };
    struct Flake {
        float x = 0, y = 0, s = 2, v = 20;
    };

    void showTitle();
    void startRun();
    void pilot(float& weight, float& brake, bool& push) const;
    void human(float& weight, float& brake, bool& push);
    void physics(float weight, float brake, bool push);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0, bool shadow = false);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    int pose() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool snap_ = true;
    bool timber_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    float legT_ = 0, anim_ = 0;
    float x_ = 0, h_ = 0, v_ = 0, att_ = 0, bed_ = 0;
    float weight_ = 0, brake_ = 0;
    float hold_ = 0, still_ = 0;
    float camX_ = 0, camH_ = 0, camS_ = 12;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    float puffT_ = 0, beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    int puffN_ = 0;
    Puff puffs_[8]{};
    Flake flakes_[16]{};
};

}  // namespace sledplat
