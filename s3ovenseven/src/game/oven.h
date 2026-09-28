// S3 OVEN SEVEN — two ovens, first tally to seven, then leave.
// Cream is one loaf. Gold is two. A six is still short of the door.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace ovenseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVEN SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool sawSix() const { return sawSix_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Win, Lose };

    void toTitle();
    void begin();
    void pull(int pts);
    void burn();
    void scoreThem(int pts);
    void settle();
    void blip(float freq);
    void backdrop();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool sawSix_ = false;
    int you_ = 0;
    int them_ = 0;
    int lastPts_ = 0;
    float heat_ = 0;
    float rival_ = 0;
    float clock_ = 0;
    float pop_ = 0;
    float sayT_ = 0;
    char say_[32] = {};
};

}  // namespace ovenseven
