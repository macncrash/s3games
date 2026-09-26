// S3 GLIDER GRASS — land on the grass and come to a full stop.
// The clock is the other crew. When it runs out, they have the grass.
#pragma once
#include "art.h"
#include "console/system.h"

namespace ggrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    float crewLeft() const { return crew_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float alt() const { return h_; }
    float speed() const { return v_; }
    float vs() const { return vy_; }
    bool onGrass() const;
    // 0 title, 1 the approach, 2 over the grass, 3 the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    struct Puff {
        float x = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void pilot(float& nose, float& spoil) const;
    void physics(float nose, float spoil);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void aimCamera(bool scenic);
    void draw(float cx, float ch, float catt, bool craft);
    void sky();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void sprAnchor(const Ship& s, float sx, float sy, float destH, int pal);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    void project(float wx, float wy, float ax, float& sx, float& sy) const;
    int wingFrame(float att) const;
    void rivalAt(float& rx, float& rh) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ground_ = false;
    bool snap_ = true;
    bool spoilWas_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    int chime_ = -1;
    int puffN_ = 0;
    int lastSec_ = -1;
    float t_ = 0, race_ = 0, crew_ = 38.f;
    float x_ = 0, h_ = 0, v_ = 0, vy_ = 0, att_ = 0;
    float nose_ = 0, spoil_ = 0, hold_ = 0;
    float camX_ = 0, camH_ = 0, camS_ = 4.6f, anchorY_ = 120.f;
    float shake_ = 0, beep_ = 0, chimeT_ = 0;
    Puff puffs_[8]{};
};

}  // namespace ggrass
