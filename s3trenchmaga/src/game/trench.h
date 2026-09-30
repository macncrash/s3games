// S3 TRENCH MAGA — one magazine, one raid, one trench.
// A round has to still be in the magazine when the raid breaks.
// Firing the last round, or letting a rush cross the parapet, ends the watch.
// The bayonet stops a man on the lip and does not spend a round.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace tmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const { return rounds_; }
    int held() const { return held_; }
    float raidTime() const { return raidT_; }
    const char* result() const;
    // 0 title, 1 the raid, 2 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Raid, Pause, Won, Lost };
    enum class Fail { None, Spent, Parapet };
    enum class Kind { Rush, Peel };
    enum class Phase { Walk, Leave, Down };

    struct Foe {
        Kind kind = Kind::Rush;
        Phase phase = Phase::Walk;
        int bay = 1;
        float z = 1.f;
        float age = 0;
        bool on = true;
    };

    void beginRaid();
    void updateRaid(float dt);
    void botAct();
    void humanAct();
    void fire();
    void bayonet();
    void lose(Fail why);
    void win();
    void draw();
    void field();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void text(const std::string& s, float x, float y, float scale, int pal);
    Foe* nearest(Kind kind, float z0, float z1, int bay, bool anyBay);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int rounds_ = 0;
    int held_ = 0;
    int bay_ = 1;
    int next_ = 0;
    float t_ = 0;
    float raidT_ = 0;
    float cool_ = 0;
    float flash_ = 0;
    float stab_ = 0;
    float shake_ = 0;
    std::vector<Foe> foes_;
};

}  // namespace tmaga
