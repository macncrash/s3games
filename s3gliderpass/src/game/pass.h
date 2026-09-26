// S3 GLIDER PASS — clear the notch before the storm clock.
// Missing the end tape fails the leg, even if the saddle was already behind you.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace gliderpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER PASS"; }
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
    float stormLeft() const;
    // 0 title, 1 the climb, 2 the notch, 3 the lee side, 4 the tapes, 5 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& nose, float& spoil) const;
    void physics(float noseCmd, float spoilCmd);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void audio();
    void camera();
    void sky(float mood);
    void draw();
    void worldBand(float x0, float x1, float hTop, float hBot, const gs::Mipped& m, int pal, int fog = 0);
    void prop(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false, int fog = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0, bool shadow = false);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float cx, float cy, float destH, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    float sx(float wx) const;
    float sy(float wy) const;
    int wingFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool passOk_ = false;
    bool gate_ = false;
    bool endDone_ = false;
    bool snapCam_ = true;
    const char* why_ = "";
    const char* note_ = "";
    float noteT_ = 0;
    float t_ = 0, legT_ = 0, passT_ = 0;
    float x_ = 0, h_ = 0, v_ = 0, vy_ = 0, att_ = 0;
    float nose_ = 0, spoil_ = 0;
    float stormX_ = 0;
    float camX_ = 0, camH_ = 0, zoom_ = 8.f, anchor_ = 96.f;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    float beep_ = 0, varioT_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    int lastSec_ = -1;
    int puffN_ = 0;

    struct Puff {
        float x = 0, y = 0, life = 0;
    };
    Puff puffs_[8]{};
};

}  // namespace gliderpass
