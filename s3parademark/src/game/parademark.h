// S3 PARADEMARK — march the street between the floats.
// The gold square painted at the end is the mark. Standing on it
// finishes the mark, and that finished mark ends the cartridge.
// Reaching the curb, the crowd, or a high score is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace parademark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int lives() const { return lives_; }
    int rows() const { return rows_; }
    const char* phase() const;

private:
    enum class Mode { Title, March, Pause, Win, Dead };

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
    void botStep();
    void humanStep();
    bool danger(float x, float y, int fr) const;
    bool crossSafe(float x, float y0, float y1) const;
    void hurt();
    void finish();
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
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    int lives_ = 3;
    int rows_ = 0;
    int frame_ = 0;
    int age_ = 0;
    int hold_ = 0;
    int inv_ = 0;
    int flash_ = 0;
    int fan_ = -1;
    int lastBeat_ = -1;
    float px_ = 160.f;
    float py_ = 200.f;
    float face_ = 1.f;
    float commitX_ = 0.f;
    float commitY_ = 0.f;
    float slide_ = 1.f;
    bool committed_ = false;
};

}  // namespace parademark
