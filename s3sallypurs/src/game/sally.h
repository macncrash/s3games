// S3 SALLY — one sortie from the gate. Be the last machine still running.
#pragma once

#include "console/gfx.h"
#include "console/system.h"

namespace sally {

enum Pal {
    PAL_HUD = 0,
    PAL_GATE = 1,
    PAL_YOU = 2,
    PAL_A = 3,
    PAL_B = 4,
    PAL_C = 5,
    PAL_D = 6,
    PAL_RUT = 7,
    PAL_SMOKE = 8,
    PAL_WIN = 9
};

struct Art {
    gs::Mipped you, rival[4], rut, smoke, gate, lamp;
    gs::Mipped glyph[96];
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the sally, 2 a machine stalls, 3 the field has gone quiet
    int marker() const;

private:
    enum class Mode { Title, Run, Victory, Dead };

    struct Rival {
        int lane = 0;
        float z = 0.5f;
        bool live = true;
    };
    struct Hazard {
        int lane = 0;
        float z = 1.f;
        float prev = 1.f;
        bool live = false;
    };

    void buildArt();
    void paintRoad();
    void begin();
    void update(float dt);
    void botPick();
    void draw();
    void project(float lane, float z, float& x, float& y, float& h) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void banner(const char* s, float y, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE SALLY STALLED";
    int score_ = 0;
    int hull_ = 2;
    int stalled_ = 0;
    int stallMark_ = 0;
    int titleTicks_ = 0;
    int script_ = 0;
    float t_ = 0;
    float scroll_ = 0;
    float bend_ = 0;
    int lane_ = 1;
    float showLane_ = 1;
    bool stick_ = false;
    Rival pack_[4]{};
    Hazard pits_[6]{};
};

}  // namespace sally
