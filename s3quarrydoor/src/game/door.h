// S3 QUARRY DOOR — hold the quarry gate for three minutes.
// A haul you miss walks the gate. The watch ends if it opens.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace quarry {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUARRY DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int openPct() const { return openPct_; }
    int secondsLeft() const { return secondsLeft_; }

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Haul {
        int at = 0;
        int dir = 1;
    };

    void begin();
    void buildHauls();
    void act();
    void playTick();
    void endTick();
    void draw();
    void sprite(const gs::Mipped& m, float x, float y, float h, int pal, bool flip);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void tone(int ch, float freq, float vol);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool planted_ = false;
    int age_ = 0;
    int play_ = 0;
    int lean_ = 0;
    int haulIx_ = 0;
    int flash_ = 0;
    int shake_ = 0;
    int toneT_ = 0;
    int openPct_ = 0;
    int secondsLeft_ = 180;
    float gap_ = 0;
    std::vector<Haul> hauls_;
};

}  // namespace quarry
