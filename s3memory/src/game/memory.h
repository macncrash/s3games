// S3 MEMORY — match the table before the clock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace memo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MEMORY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pairs() const { return pairs_; }
    int turns() const { return turns_; }
    float clockLeft() const { return clockFrames_ / 60.f; }

private:
    enum class Mode { Title, Study, Cover, Ready, Play, Pause, Win, Lose };

    struct Card {
        int face = 0;
        bool up = false;
        bool matched = false;
        float show = 0;
    };

    static constexpr int kClock = 90 * 60;

    void freshDeal();
    void dealIn(int studyFrames);
    void mapSlots();
    void physics();
    void readPad();
    void botAct();
    void nudge(int& dx, int& dy);
    void moveCursor(int dx, int dy);
    bool tryFlip(int idx);
    bool closing() const;
    void approach(Card& c);
    void clockTick();
    void audio();
    void win();
    void lose();
    void blip(float freq);
    void matchChime();
    void miss();
    void draw();
    void felt();
    void drawBar();
    void drawTitle();
    void drawTable();
    void drawWood();
    void chrome();
    void spr(const gs::Mipped& m, float cx, float cy, float h, float xscale, int pal, bool shadow);
    void bar(float x, float y, float w, float h, int pal);
    void cell(int i, float& cx, float& cy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int clockSec() const;
    uint32_t rndu();

    gs::System* sys_ = nullptr;
    Art art_{};
    Card cards_[CARDS];
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int pairs_ = 0;
    int turns_ = 0;
    int cursor_ = 0;
    int clockFrames_ = kClock;
    int studyFrames_ = 0;
    int ready_ = 0;
    int shut_ = 0;
    int nOpen_ = 0;
    int open_[2] = {};
    int slotA_[PAIRS] = {};
    int slotB_[PAIRS] = {};
    int botFace_ = 0;
    int botPhase_ = 0;
    int holdX_ = 0, holdY_ = 0, holdXT_ = 0, holdYT_ = 0;
    int beep_ = 0;
    int tick_ = 0;
    int fanFrame_ = -1;
    int lastSec_ = -1;
    uint32_t rng_ = 1;
};

}  // namespace memo
