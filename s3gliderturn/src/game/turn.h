// S3 GLIDER TURN — make the three turns without tipping.
// Each pylon wants a bank through the sector. Past the tip, the wing drops.
// Missing the end gate fails the leg, turns or not.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace gliderturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER TURN"; }
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
    float bank() const { return bank_; }
    int turns() const { return next_; }
    float arc() const { return arc_; }
    // 0 title, 1 the glide, 2 banking a turn, 3 running to the end, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(float& nose, float& spoil, float& stick) const;
    void physics(float nose, float spoil, float stick);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    int wingFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool snap_ = true;
    const char* why_ = "";
    const char* banner_ = "";
    const char* toast_ = "";
    float legT_ = 0, anim_ = 0;
    float x_ = 0, h_ = 0, v_ = 0, vy_ = 0, bank_ = 0;
    float nose_ = 0, spoil_ = 0, stick_ = 0;
    float overB_ = 0, pegT_ = 0, arc_ = 0;
    float camX_ = 0, camH_ = 0, camS_ = 4.f;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    float beep_ = 0, toastT_ = 0, chimeT_ = 0;
    int next_ = 0;
    int chime_ = -1;
    int puffN_ = 0;
    Puff puffs_[8]{};
};

}  // namespace gliderturn
