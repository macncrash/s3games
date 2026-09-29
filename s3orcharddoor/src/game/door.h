// S3 ORCHARD DOOR — one orchard. Hold the door for three minutes.
// Opening it early, or letting the seam run, ends the watch.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace orcharddoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ORCHARD DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int braces() const { return braces_; }
    int fruit() const { return fruit_; }
    // 0 title, 1 the door, 2 held, 3 opened
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum Kind { SHOVE_L = 0, SHOVE_R = 1, LATCH = 2, FRUIT = 3 };

    struct Beat {
        int start = 0;
        int len = 0;
        int kind = 0;
        bool met = false;
    };
    struct Mote {
        float x, y, vx, vy, life;
    };

    void bootTitle();
    void begin();
    void finish(bool held);
    void layBeats();
    void applyBot();
    void updatePlay();
    void serviceAudio();
    int liveBeat() const;
    void sky();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    const char* kindName(int k) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool early_ = false;
    int age_ = 0;
    int titleAge_ = 0;
    int braces_ = 0;
    int fruit_ = 0;
    int missed_ = 0;
    int endAge_ = 0;
    int lean_ = 0;
    float seam_ = 0;
    float shake_ = 0;
    int beatCount_ = 0;
    Beat beats_[48]{};
    Mote motes_[16]{};
};

}  // namespace orcharddoor
