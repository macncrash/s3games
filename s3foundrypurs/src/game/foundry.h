// S3 FOUNDRY PURSUIT — one foundry. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace foundryp {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stalled() const { return stalled_; }
    int fleet() const { return kFleet; }
    int heat() const { return heat_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the floor is live, 2 a machine just seized, 3 the foundry is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    static constexpr int kFleet = 6;

    struct Rival {
        int rail = 1;
        int kind = 0;
        int hp = 1;
        float x = 330.f;
        float cool = 0.9f;
        bool running = true;
    };
    struct Shot {
        int rail = 0;
        float x = 0;
        float vx = 0;
        bool ours = false;
        bool live = true;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void bootFloor();
    void begin();
    void update();
    void spawnDue();
    void quench();
    void rivalShot(Rival& r);
    void seize(Rival& r);
    void chill();
    void winFloor();
    void loseFloor(const char* why);
    void botAct();
    void puff(float x, float y, int n);
    void tickMotes();
    void blip(int ch, float freq, float vol);
    void serviceAudio();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    float railY(int rail) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int playF_ = 0;
    int rail_ = 1;
    int heat_ = 8;
    int stalled_ = 0;
    int spawnIx_ = 0;
    int cool_ = 0;
    int seize_ = 0;
    int flicker_ = 0;
    float hold_ = 0;
    float blip_ = 0;
    const char* reason_ = "";
    std::vector<Rival> rivals_;
    std::vector<Shot> shots_;
    Mote motes_[24] = {};
};

}  // namespace foundryp
