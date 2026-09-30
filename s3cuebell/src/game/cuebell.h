// S3 CUE BELL — one cloth, one cue, a brass bell over the far cushion.
// A stroke rings the bell only if the tip meets the ball in the window.
// A thin stroke dies. Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace cuebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CUE BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Roll, Dead, Ring, Lose };

    void begin();
    void stroke();
    void dieTry(const char* why);
    void ring();
    void botPlay();
    bool inWindow() const;
    float meter() const;
    float cueTip() const;

    void hush();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool botHeld_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    const char* why_ = "";
    float phase_ = 0;
    float modeT_ = 0;
    float ballX_ = 118.f;
    float roll_ = 0;
    float toneT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.1f;
};

}  // namespace cuebell
