// S3 RIDGE MAGA — one magazine, one raid, the crest between them.
// A round spent on the shoulder, or the last round spent at all, loses the ridge.
// The clock has to die with a live round still in the magazine.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace rmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE MAGA"; }
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
    enum class Fail { None, Spent, Through };
    enum class Kind { Commit, Peel };
    enum class Phase { Walk, Peel, Down };

    struct Foe {
        Kind kind = Kind::Commit;
        Phase phase = Phase::Walk;
        float u = 0, z = 0, peelZ = 0, age = 0, side = 1;
        bool on = true;
        bool warned = false;
    };
    struct Puff {
        float u, z, t;
    };
    struct Casing {
        float x, y, vx, vy, t;
    };
    struct Spot {
        float x, y, h;
        int fog;
    };
    struct Blob {
        float z;
        float u;
        float base;
        const gs::Mipped* img;
        int pal;
        bool flip;
        bool shade;
    };

    void beginRaid();
    void updateRaid(float dt);
    void botAct(float dt);
    void humanAct(float dt);
    void pull();
    void lose(Fail why);
    void win();
    void draw();
    void tickFx(float dt);
    void shotSound(bool hit);
    Spot spot(float u, float z, float base) const;
    float bend(float row) const;
    float halfW(float row) const;
    float rowAt(float z) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool hittable(const Foe& f) const;
    Foe* acquire();
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wantFire_ = false;
    int rounds_ = 5;
    int stopped_ = 0;
    int next_ = 0;
    int fanStep_ = -1;
    float t_ = 0, raidT_ = 0, bolt_ = 0, kick_ = 0, flash_ = 0, shake_ = 0, fanT_ = 0;
    float u_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Foe> foes_;
    std::vector<Puff> puffs_;
    std::vector<Casing> casings_;
};

}  // namespace rmaga
