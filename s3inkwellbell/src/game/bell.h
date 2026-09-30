// S3 INKWELL BELL — catch the ink. The bell rings at eight marks.
// Leave when it rings before the third try dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace inkwellbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 INKWELL BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int marks() const { return marks_; }
    int tries() const { return tries_; }

private:
    enum class Mode { Title, Play, Miss, Ring, Pause, Over };

    void begin();
    void spawn();
    void catchInk();
    void miss();
    void leave();
    void chime(float freq);
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool drop_ = false;
    int marks_ = 0;
    int tries_ = kTries;
    int anim_ = 0;
    float wellX_ = 160.f;
    float dropX_ = 160.f;
    float dropY_ = 20.f;
    float dropVy_ = 50.f;
    float swing_ = 0.f;
    uint32_t rng_ = 0xB411u;
};

}  // namespace inkwellbell
