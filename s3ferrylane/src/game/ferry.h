// S3 FERRY LANE — stay inside the buoyed channel for the whole crossing.
// The clock is the other crew. If it runs out, the slip is theirs.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace ferrylane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY LANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float z() const { return z_; }
    float lateral() const { return lat_; }
    float speed() const { return spd_; }
    float margin() const;
    float crewLeft() const { return crew_; }
    // 0 title, 1 the crossing, 2 mid-channel, 3 the slip, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    float steerInput() const;
    void pilotSteer(float& steer) const;
    void physics(float steer);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void draw();
    void backdrop();
    void channel();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool near_ = false;
    bool tight_ = false;
    bool slip_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    float legT_ = 0, anim_ = 0, crew_ = 26.f;
    float z_ = 0, lat_ = 0, spd_ = 0, steer_ = 0;
    float crewZ_ = 0;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    int crewSec_ = 99;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace ferrylane
