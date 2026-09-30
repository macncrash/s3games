// S3 MASKTAPE — gild the half-mask so the drawer matches the tape.
// Lay CROWN, then LIP, then CHEEK. The other beds stay out of the till.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace masktape {

constexpr int kSlots = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MASKTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool matched() const;
    const char* reason() const { return reason_; }
    const char* slotName(int i) const;
    const char* tapeName(int i) const;
    // 0 title, 1 gilding, 2 left
    int marker() const;

private:
    enum class Mode { Title, Gild, Lead, Leave, Lost, Over };

    struct Input {
        bool lay = false;
        bool spoil = false;
        bool start = false;
    };

    static constexpr int kPeriod = 72;
    static constexpr int kSpan = 12;
    static constexpr int kWide = 10;
    static constexpr int kFaults = 3;
    static constexpr int kTapeGate[kSlots] = {1, 3, 5};

    void begin();
    void startGild();
    void lay();
    void endSweep();
    void toLead();
    void toLeave();
    void toLost(const char* why);
    void finishLeave();
    void stepPlay(const Input& in);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    int gate() const;
    bool inBed() const;
    int nextOpen() const;
    const char* gateName(int g) const;
    bool tapeGate(int g) const;

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    int clock_ = 0;
    int titleWait_ = 0;
    int show_ = 0;
    int flash_ = 0;
    int filled_ = 0;
    int faults_ = 0;
    int held_[kSlots] = {-1, -1, -1};
    bool swept_ = false;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float beep_ = 0;
};

}  // namespace masktape
