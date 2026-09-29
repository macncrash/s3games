// S3 STRIKER GOLD — a short carnival tower.
// Cream marks score their face. Only the gold counts double.
// Leave when the line is met and the undoubled faces are still short.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace strikergold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKER GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int swings() const { return used_; }
    int line() const { return kLine; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Ready, Strike, Rise, Show, Win, Lose };

    void begin();
    void launch(float power);
    void settle();
    void judge();
    void botPlay();
    float meter() const;
    int pose() const;
    int markOf(float power) const;

    void blip(bool high);
    void thunk();
    void chime();
    void whistle(float h);

    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void img(const gs::Image& im, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void meterBar(int row);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    int leftSwings_ = 3;
    int used_ = 0;
    int landed_ = -1;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    float phase_ = 0;
    float power_ = 0;
    float apex_ = 0;
    float puck_ = 0;
    float puckV_ = 0;
    float modeT_ = 0;
    float bellT_ = 0;
    float flash_ = 0;
    const char* why_ = "";
};

}  // namespace strikergold
