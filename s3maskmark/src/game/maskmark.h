// S3 MASKMARK — one blank mask on the bench.
// Four cuts: brow, left eye, right eye, mouth. Each locks only in the groove.
// The seal then seats. That finished mark ends the cartridge. A smear stays open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace maskmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MASKMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool sealed() const { return sealed_; }
    int cuts() const { return locked_; }
    const char* markName() const;

private:
    enum class Mode { Title, Cut, Stamp, Done };

    struct Input {
        bool cut = false;
        bool start = false;
    };

    static constexpr int kCuts = 4;

    void begin();
    void lockCut();
    void smear();
    void seat();
    void finish();
    void stepPlay(const Input& in);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    float needle() const;
    bool inGroove() const;
    void toneAt(float freq, float vol);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();
    void cutLine(int i, float& x0, float& y0, float& x1, float& y1) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    int locked_ = 0;
    int smearN_ = 0;
    int flash_ = 0;
    int titleWait_ = 0;
    int stampT_ = 0;
    int doneWait_ = 0;
    int clock_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool sealed_ = false;
    bool released_ = true;
    float beep_ = 0;
};

}  // namespace maskmark
