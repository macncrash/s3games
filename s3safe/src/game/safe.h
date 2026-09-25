// S3 SAFE — three dials, and the room has the numbers.
#pragma once
#include <random>

#include "console/system.h"
#include "game/art.h"

namespace vault {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SAFE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool solved() const;
    int dial(int i) const { return dial_[i]; }
    int combo(int i) const { return code_[i]; }

private:
    enum class Mode { Title, Play, Opening, Open };

    void deal();
    void botAct();
    void human();
    void nudgeSel(int d);
    void nudgeDial(int d);
    void pull();
    void clickAt(float freq);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(const gs::Mipped& m, float x, float y, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int code_[3] = {};
    int dial_[3] = {};
    int sel_ = 0;
    int t_ = 0;
    int openT_ = 0;
    int shake_ = 0;
    int wrong_ = 0;
    int beep_ = 0;
    int thud_ = 0;
    int hold_[4] = {};
    float slide_ = 0;
    std::mt19937 rng_{1};
};

}  // namespace vault
