// S3 CISTERN MAGA — one cistern, one magazine, one raid.
// A round only stops a raider still on the coping. A shot into the water,
// or at a wader turning back, is a miss and still leaves the magazine.
// The watch holds only if a round is left when the raid clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace cmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERN MAGA"; }
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
    enum class Fail { None, Spent, Rim };
    enum class Kind { Commit, Wade };
    enum class Phase { Climb, Turn, Sunk };

    struct Foe {
        Kind kind = Kind::Commit;
        Phase phase = Phase::Climb;
        int sector = 1;
        float along = 0;
        float age = 0;
        bool on = true;
    };

    void beginRaid();
    void updateRaid(float dt);
    void botAct();
    void humanAct();
    void pull();
    void lose(Fail why);
    void win();
    void draw();
    void place(const Foe& f, float& x, float& y, float& h) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void text(const std::string& s, float x, float y, float scale, int pal);
    Foe* onCoping(int sector);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int rounds_ = 0;
    int stopped_ = 0;
    int sector_ = 1;
    int next_ = 0;
    int misses_ = 0;
    float t_ = 0;
    float raidT_ = 0;
    float cool_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    bool lastHit_ = false;
    std::vector<Foe> foes_;
};

}  // namespace cmaga
