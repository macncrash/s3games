// S3 GLIDER LANE — stay between the rails for the whole leg.
// The clock is the other crew. If it runs out, the leg is theirs.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace glane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER LANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float alt() const { return h_; }
    float speed() const { return v_; }
    float vs() const { return vy_; }
    float margin() const;
    float crewLeft() const { return crew_; }
    // 0 title, 1 the glide, 2 mid-leg, 3 the finish gate, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(float& nose, float& spoil) const;
    void physics(float nose, float spoil);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, int fog = 0);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    int frameFor(float att) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool spoilArm_ = false;
    bool near_ = false;
    bool tight_ = false;
    bool gate_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    float legT_ = 0, anim_ = 0, crew_ = 18.f;
    float x_ = 0, h_ = 0, v_ = 0, vy_ = 0, att_ = 0;
    float nose_ = 0, spoil_ = 0;
    float crewX_ = 0;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    int crewSec_ = 99;
    int puffN_ = 0;
    Puff puffs_[6]{};
};

}  // namespace glane
