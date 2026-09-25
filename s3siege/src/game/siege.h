// S3 SIEGE — you are the wall. Stop the rams before the gate breaks.
#pragma once

#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace siege {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SIEGE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int gate() const { return gate_; }
    int score() const { return score_; }
    int breaches() const { return breaches_; }
    // 0 title, 1 the wall, 2 the gate holds, 3 the gate breaks
    int marker() const;

private:
    enum class Mode { Title, Siege, Pause, Won, Lost };

    struct Ram {
        float x = 0, y = 0, speed = 0, half = 16, age = 0;
        bool heavy = false;
        bool alive = false;
    };
    struct Puff {
        float x = 0, y = 0, t = 0, h = 8;
        bool spark = false;
    };

    void newSiege();
    void spawnDue();
    void botThink(float& slide, bool& brace, bool& drop);
    bool afford(int sec, int pri) const;
    int stoneVictim() const;
    void tryDrop();
    void stepStone(float dt);
    void stepRams(float dt);
    void meet(Ram& ram);
    void scar(int n);
    void boom(float x, bool bad);
    void settle();
    void beginWin();
    void beginLoss();
    void note(const char* s, int pal);
    void fanfare();
    void blip(float freq, float vol);
    void quiet();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool top);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void banner(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Siege;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool through_ = false;
    bool braced_ = false;
    bool stone_ = false;
    int gate_ = 5;
    int score_ = 0;
    int stopped_ = 0;
    int resolved_ = 0;
    int spawned_ = 0;
    int breaches_ = 0;
    int next_ = 0;
    int siegeFrame_ = 0;
    int fanStep_ = -1;
    int botDetour_ = -1;
    bool fanOn_ = false;
    float anim_ = 0;
    float titleT_ = 0;
    float bannerT_ = 0;
    float stoneX_ = 0, stoneY_ = 0;
    float stoneCd_ = 0;
    float px_ = 160;
    float flash_ = 0, shake_ = 0, beep_ = 0, noteT_ = 0, fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    const char* note_ = "";
    int notePal_ = PAL_GOLD;
    std::vector<Ram> rams_;
    std::vector<Puff> puffs_;
};

}  // namespace siege
