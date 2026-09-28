// S3 MILL DOOR — the mill race. Hold the door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace mill {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILL DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float gap() const { return gap_; }
    // 0 title, 1 the door, 2 held, 3 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum class Phase { Quiet, Tele, Strike };

    struct Cue {
        int frame;
        int kind;
        int side;
    };
    struct Mote {
        float x, y, vy, life;
    };

    void begin();
    void toTitle();
    void layCues();
    void update();
    void draw();
    void backdrop(float shx);
    void lamp();
    void note(float freq);
    void fanfare();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false, int fog = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Quiet;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool barred_ = false;
    bool chocked_ = false;
    int age_ = 0;
    int gate_ = 0;
    int lean_ = 0;
    int kind_ = 0;
    int side_ = 0;
    int cueNext_ = 0;
    int cueCount_ = 0;
    int fan_ = -1;
    float gap_ = 0;
    float shake_ = 0;
    float wheel_ = 0;
    float fanT_ = 0;
    float noteT_ = 0;
    Cue cues_[64]{};
    Mote motes_[20]{};
};

}  // namespace mill
