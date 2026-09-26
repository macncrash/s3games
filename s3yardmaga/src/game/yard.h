// S3 YARD MAGA — one magazine, one raid, the scale-house window between them.
// A cutter only drops in the floodlight. A hand ends the watch.
// The clock has to die with a live round still in the magazine.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace yardmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const { return rounds_; }
    int stopped() const { return stopped_; }
    float raidTime() const { return raidT_; }
    const char* result() const;
    std::string trace() const;
    // 0 title, 1 the raid, 2 the raid has ended
    int marker() const;

private:
    enum class Mode { Title, Raid, Pause, Won, Lost };
    enum class Fail { None, Spent, Sill, Hand };
    enum class Kind { Cutter, Hand };
    enum class Phase { Walk, Peel, Clear, Dead };

    struct Man {
        Kind kind = Kind::Cutter;
        Phase phase = Phase::Walk;
        float x = 0, z = 0, walk = 0, dead = 0;
        bool warned = false;
    };
    struct Casing {
        float x = 0, y = 0, vx = 0, vy = 0, t = 0;
    };
    struct Spark {
        float x = 0, y = 0, t = 0;
    };

    void beginRaid();
    void updateRaid(float dt);
    void botAct(float dt);
    void humanAim(float dt);
    void pull();
    void kill(Man& m);
    void lose(Fail why);
    void win();
    void tickFx(float dt);
    void draw();
    void world(float x, float y, float z, float& sx, float& sy, float& s) const;
    float bendPx(float z) const;
    int fogFor(float z) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false,
             bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool fireEdge() const;
    Man* acquire(bool cuttersOnly);
    const char* hint() const;
    int hintPal() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wantFire_ = false;
    int rounds_ = 6;
    int stopped_ = 0;
    int next_ = 0;
    int fanStep_ = -1;
    float t_ = 0, raidT_ = 0, bolt_ = 0, kick_ = 0, flash_ = 0, shake_ = 0, fanT_ = 0;
    float aimX_ = 160, aimY_ = 108;
    float shx_ = 0, shy_ = 0;
    float flashX_ = 160, flashY_ = 120;
    std::vector<Man> men_;
    std::vector<Casing> casings_;
    std::vector<Spark> sparks_;
};

}  // namespace yardmaga
