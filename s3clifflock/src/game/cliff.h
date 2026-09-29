// S3 CLIFF LOCK — take the cliff and pass the lock without scraping a gate.
// The other crew is the clock. Missing the far mark loses the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace clifflock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFF LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float speed() const { return speed_; }
    // 0 title, 1 approach, 2 lower gate, 3 chamber, 4 climb, 5 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Gate {
        float x = 0;
        float open = 0;
        bool passed = false;
    };

    void showTitle();
    void startRun();
    void pilot(float& throttle, float& brake, float& steer) const;
    void physics(float throttle, float brake, float steer);
    void gates(float dt);
    void checkGate(Gate& g);
    void finishLine();
    void win();
    void fail(const char* why);
    void audio();
    void draw();
    void sky();
    void prop(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float roadY(float wx) const;
    float sx(float wx) const;
    float sy(float wy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    const char* note_ = "";
    float noteT_ = 0;
    float t_ = 0, legT_ = 0;
    float x_ = 0, lat_ = 0, speed_ = 0;
    float camX_ = 0;
    Gate lo_{}, hi_{};
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace clifflock
