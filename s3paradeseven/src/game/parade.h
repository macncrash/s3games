// S3 PARADE SEVEN — march the street between the floats.
// Each safe arrival at the far curb is one. First to seven leaves.
// A high score that is still short of seven is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace paradeseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int rows() const { return rows_; }
    const char* phase() const;

private:
    enum class Mode { Title, March, Pause, Win, Dead };

    struct Who {
        float x = 160.f;
        float y = 202.f;
        float face = 1.f;
        float commitX = 0.f;
        float commitY = 0.f;
        float slide = 1.f;
        bool committed = false;
        int inv = 0;
        int flash = 0;
    };

    struct Lane {
        float y;
        float speed;
        float spacing;
        float width;
        float phase;
        int pal;
    };

    void toTitle();
    void begin();
    void logic();
    void stepWho(Who& w, bool rival);
    void humanStep(Who& w);
    bool danger(float x, float y, int fr) const;
    bool crossSafe(const Who& w, float x, float y0, float y1) const;
    void arrive(bool you);
    void bump(Who& w, bool you);
    void audio();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::March;
    Lane lanes_[4] = {};
    Who youW_{};
    Who themW_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int them_ = 0;
    int rows_ = 0;
    int frame_ = 0;
    int age_ = 0;
    int fan_ = -1;
    int lastBeat_ = -1;
};

}  // namespace paradeseven
