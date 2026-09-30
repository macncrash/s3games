// S3 MASK GOLD — lay leaf on a half-mask.
// Gold leaf counts double. Cream glaze counts one.
// Leave when the line clears only because of that double.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace maskgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MASK GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int line() const { return kLine; }
    int slot() const { return slot_; }
    // 0 title, 1 laying, 2 left
    int marker() const;

private:
    enum class Mode { Title, Sweep, Leave, Fail, Done };
    enum class Kind { Gold, Cream };

    struct Input {
        bool cut = false;
        bool skip = false;
        bool start = false;
    };

    static constexpr int kSlots = 6;
    static constexpr int kLine = 8;
    static constexpr int kPeriod = 42;

    void begin();
    void take();
    void pass();
    void smear();
    void leave();
    void stepPlay(const Input& in);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    float needle() const;
    bool inGroove() const;
    bool clears() const;
    Kind kindAt(int i) const;
    void slotLine(int i, float& x0, float& y0, float& x1, float& y1) const;
    void toneAt(float freq, float vol);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    int slot_ = 0;
    int gold_ = 0;
    int cream_ = 0;
    int bare_ = 0;
    int score_ = 0;
    int laid_[kSlots] = {};  // 0 empty, 1 gold, 2 cream
    int flash_ = 0;
    int titleWait_ = 0;
    int leaveWait_ = 0;
    int clock_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool released_ = true;
    float beep_ = 0;
};

}  // namespace maskgold
