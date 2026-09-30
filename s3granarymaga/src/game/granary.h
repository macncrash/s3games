// S3 GRANARY MAGA — the loft has one magazine. The raid has the yard.
// Sack-carriers peel into the ricks. Ram-men stay on the grain door.
// The magazine still has to hold a round when the raid clock dies.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace grmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GRANARY MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const { return rounds_; }
    int stopped() const { return stopped_; }
    float raidTime() const { return raidT_; }
    const char* result() const;
    // 0 title, 1 the raid, 2 the raid has ended
    int marker() const;

private:
    enum class Mode { Title, Raid, Pause, Won, Lost };
    enum class Fail { None, Spent, Door };
    enum class Kind { Ram, Sack };
    enum class Phase { Walk, Peel, Down };

    struct Foe {
        Kind kind = Kind::Ram;
        Phase phase = Phase::Walk;
        int lane = 0;
        int side = 1;
        float z = 1.05f;
        float age = 0;
        bool on = true;
    };

    void beginRaid();
    void updateRaid(float dt);
    void botAct();
    void humanAct();
    void fire();
    void lose(Fail why);
    void win();
    void draw();
    void yard();
    void place(const Foe& f, float& x, float& y, float& h) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    Foe* inLane(int lane);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int rounds_ = 0;
    int stopped_ = 0;
    int lane_ = 0;
    int next_ = 0;
    float t_ = 0;
    float raidT_ = 0;
    float cool_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    std::vector<Foe> foes_;
};

}  // namespace grmaga
