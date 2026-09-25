// S3 FORT — hold the gate until the clock dies.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace fort {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FORT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int gate() const { return gate_; }
    int score() const { return score_; }
    int leaks() const { return leaks_; }
    int kills() const { return kills_; }
    float siege() const { return siege_; }
    // 0 title, 1 the wall, 2 the clock dies, 3 the gate falls
    int marker() const;

private:
    enum class Mode { Title, Siege, Pause, Won, Lost, Over };
    enum class Kind { Raider, Shield, Ram };

    struct Foe {
        Kind kind = Kind::Raider;
        float x = 0, z = 0, lane = 0, speed = 1, radius = 0.2f;
        float phase = 0, freq = 1, weave = 0, age = 0, flash = 0;
        int hp = 1, dmg = 10, score = 100;
        bool alive = true;
    };
    struct Bolt {
        float x = 0, z = 0;
        bool alive = true;
    };
    struct Puff {
        float x = 0, z = 0, t = 0, h = 8;
        bool spark = false;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
        bool bad = false;
    };

    void newGame();
    void update(float dt);
    void botAim(float& slide, bool& fire, bool& braceDown);
    void spawn(Kind kind, float x, float z, float speedMul);
    void spawnRush(int which);
    void loose();
    void braceGate();
    void beginWin();
    void beginLoss();
    void fanfare();
    void blip(float freq, float vol);
    void quiet();
    int inbound(int index) const;
    int pickFocus() const;
    uint32_t rnd();
    float frnd();
    void draw();
    void drawFoe(const Foe& f);
    void project(float x, float z, float& sx, float& sy) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void puffAt(float x, float z, float h, bool spark);
    void popAt(float sx, float sy, int pts, bool bad);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Siege;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool drone_ = false;
    int gate_ = 100;
    int score_ = 0;
    int leaks_ = 0;
    int kills_ = 0;
    int attempt_ = 1;
    int lastLane_ = -1;
    int lastSec_ = 99;
    int fanStep_ = -1;
    int rushMask_ = 0;
    float age_ = 0;
    float siege_ = 0;
    float playerX_ = 0;
    float fireCd_ = 0;
    float braceCd_ = 0;
    float spawnCd_ = 0;
    float aim_ = 0;
    float braceT_ = 0;
    float shake_ = 0;
    float flash_ = 0;
    float banner_ = 0;
    float fanT_ = 0;
    float beep_ = 0;
    float shx_ = 0, shy_ = 0;
    uint32_t rng_ = 1;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
    std::vector<Puff> puffs_;
    std::vector<Pop> pops_;
};

}  // namespace fort
