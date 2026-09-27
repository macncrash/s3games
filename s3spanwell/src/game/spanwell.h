// S3 SPANWELL — one span. Keep the well standing through three waves.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace spanwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPANWELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int wave() const { return wave_; }
    bool playing() const { return mode_ == Mode::Play; }
    bool surged() const { return mode_ == Mode::Play && phase_ == Phase::Wave && waveT_ > 0.4f; }

private:
    enum class Mode { Title, Play, Fell, Stood };
    enum class Phase { Calm, Wave };

    void newGame();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float surge() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Calm;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    float clock_ = 0;
    float waveT_ = 0;
    float calmT_ = 0;
    float endT_ = 0;
    float wx_ = 0;  // well along the span, -1..1, past the ends it falls
    float wv_ = 0;
    float px_ = 0;  // keeper
    float shake_ = 0;
    bool bracing_ = false;
    bool left_ = false;
    bool right_ = false;
};

}  // namespace spanwell
