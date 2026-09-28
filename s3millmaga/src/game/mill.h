// S3 MILL MAGA — you have the mill. One magazine. One raid.
// A round has to still be in the magazine when the raid breaks.
// Spending the last round, or letting them through the mill door, loses the mill.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace mmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILL MAGA"; }
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
    enum class Kind { Commit, Peel };
    enum class Phase { Walk, Leave, Down };

    struct Foe {
        Kind kind = Kind::Commit;
        Phase phase = Phase::Walk;
        int lane = 1;
        float z = 1.f;
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
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void text(const std::string& s, float x, float y, float scale, int pal);
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
    int lane_ = 1;
    int next_ = 0;
    float t_ = 0;
    float raidT_ = 0;
    float cool_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    float sail_ = 0;
    std::vector<Foe> foes_;
};

}  // namespace mmaga
