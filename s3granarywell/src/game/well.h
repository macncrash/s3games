// Keep the granary well standing through three waves of stones.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace granary {

class Game : public gs::Cart {
public:
    const char* title() const override { return "GRANARY WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int wave() const { return wave_; }
    int through() const { return through_; }
    int stopped() const { return stopped_; }
    int wellHp() const { return wellHp_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 between waves, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Banner, Victory, Defeat };

    struct Spawn {
        float t;
        float x;
    };
    struct Stone {
        float x, y, vx, vy;
        bool live;
    };
    struct Puff {
        float x, y, t;
    };

    void bootWatch();
    void prepareWave();
    void update(float dt);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(int ch, float freq, float vol);
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int wellHp_ = 4;
    int stopped_ = 0;
    int through_ = 0;
    int score_ = 0;
    float t_ = 0;
    float px_ = 160;
    float swing_ = 0;
    float swingCd_ = 0;
    float bannerT_ = 0;
    int spawnI_ = 0;
    uint32_t rng_ = 0x6E11u;
    const char* reason_ = "THE WELL FELL";
    std::vector<Spawn> spawns_;
    std::vector<Stone> stones_;
    std::vector<Puff> puffs_;
};

}  // namespace granary
