// S3 SUBPASS — take the sub through the pass before the other crew's storm clock.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace subpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUBPASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int crewPct() const { return crewPct_; }

private:
    enum class Mode { Title, Run, Win, Lose, Pause };

    struct Mine {
        float x, y;
    };
    struct Fish {
        float x, y, v;
        int kind;
    };

    void beginRun();
    void updateRun();
    void draw();
    void hud(int col, int row, const std::string& s);
    void hudC(int row, const std::string& s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool feet = false);
    float rnd();
    float ceilingAt(float x) const;
    float floorAt(float x) const;
    void botSteer(float& want) const;
    void hurt();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Run;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int hull_ = 4;
    int crewPct_ = 0;
    float px_ = 80;
    float py_ = 112;
    float clock_ = 0;
    float stun_ = 0;
    float inv_ = 0;
    float t_ = 0;
    float sonar_ = 0;
    uint32_t rng_ = 0x5B11u;
    std::vector<Mine> mines_;
    std::vector<Fish> fish_;
};

}  // namespace subpass
