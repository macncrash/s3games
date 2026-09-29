// S3 BOARD SEVEN — a short board. First seat to seven plugs wins.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace boardseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOARD SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Win, Lose };

    void open();
    void nextCall();
    void plug(int jack);
    void botAct();
    void human();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(float x, float y, float w, float h, int pal);
    float jackX(int i) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int mark_ = 0;
    int cursor_ = 0;
    int you_ = 0;
    int them_ = 0;
    int life_ = 0;
    int t_ = 0;
    int act_ = 0;
    int beep_ = 0;
    int flash_ = 0;
    uint32_t rng_ = 0xB047u;
};

}  // namespace boardseven
