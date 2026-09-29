// S3 ANVIL BELL — strike the anvil. The bell rings before the third try dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace anvilbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ANVIL BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int dead() const { return dead_; }
    int attempt() const { return attempt_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Try, Rise, Gap, Over };

    bool open() const;
    void begin();
    void strike(bool hit);
    void killTry();
    void blip(float freq);
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int dead_ = 0;
    int attempt_ = 0;
    int age_ = 0;
    int anim_ = 0;
    float slugY_ = 0;
    const char* why_ = "";
};

}  // namespace anvilbell
