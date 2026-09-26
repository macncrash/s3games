// S3 GATE MAGA — one magazine, one raid, the gate between them.
// A round only counts on the open rush. The clock must die with a round left.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace maga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE MAGA"; }
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
    // 0 title, 1 the watch, 2 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Raid, Pause, Won, Lost };
    enum class Fail { None, Spent, Through };
    enum class Phase { Scuttle, Cover, Rush, Dead };

    struct Man {
        float x = 0, z = 0, hideZ = 0, holdDur = 0, cover = 0, dead = 0, ph = 0, side = 0;
        Phase phase = Phase::Scuttle;
        int coat = 0;
    };
    struct Spark {
        float x, y, t;
    };
    struct Casing {
        float x, y, vx, vy, t;
    };
    struct Blob {
        float z, x, worldH;
        const gs::Mipped* img;
        int pal, fog;
        bool flip;
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
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void shotSound(bool hit);
    bool rushing() const;
    bool anyone() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wantFire_ = false;
    int rounds_ = 12;
    int stopped_ = 0;
    int next_ = 0;
    int fanStep_ = -1;
    float t_ = 0, raidT_ = 0, bolt_ = 0, kick_ = 0, flash_ = 0, shake_ = 0, fanT_ = 0;
    float aimX_ = 160, aimY_ = 104;
    float shx_ = 0, shy_ = 0;
    std::vector<Man> men_;
    std::vector<Spark> sparks_;
    std::vector<Casing> casings_;
};

}  // namespace maga
