// S3 HARBOR MAGA — one harbor mouth, one magazine, one raid of cutters.
// Sink the raid and still have a round left. A dry magazine, or a cutter
// against the quay, ends it.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace harbormaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const { return rounds_; }
    int sunk() const { return sunk_; }
    float raidTime() const { return raidT_; }
    const char* result() const;
    // 0 title, 1 the raid, 2 the raid has ended
    int marker() const;

private:
    enum class Mode { Title, Raid, Won, Lost };
    enum class Fail { None, Spent, Quay };

    struct Boat {
        float x = 0, z = 0, dead = 0;
        bool sunk = false;
    };
    struct Splash {
        float x = 0, y = 0, t = 0;
    };

    void beginRaid();
    void updateRaid(float dt);
    void botAct(float dt);
    void humanAim(float dt);
    void pull();
    void lose(Fail why);
    void win();
    void draw();
    void project(float x, float z, float& sx, float& sy, float& s) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void textC(const std::string& s, float y, float scale, int pal);
    bool fireEdge();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wantFire_ = false;
    bool trigWas_ = false;
    int rounds_ = 7;
    int sunk_ = 0;
    int next_ = 0;
    float t_ = 0, raidT_ = 0, bolt_ = 0, kick_ = 0, shake_ = 0;
    float aimX_ = 160, aimY_ = 130;
    std::vector<Boat> boats_;
    std::vector<Splash> splashes_;
};

}  // namespace harbormaga
