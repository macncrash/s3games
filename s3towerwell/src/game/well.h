// S3 TOWER WELL — you have the tower. Keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace tww {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWER WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int well() const { return well_; }
    int wave() const { return wave_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the field, 2 the well stands, 3 the well fell
    int marker() const;

private:
    enum class Mode { Title, Brief, Fight, Gap, Victory, Fail, Pause };

    struct Foe {
        int lane = 0;
        int hp = 1;
        int kind = 0;  // 0 raider, 1 brute
        float x = 0;
        float speed = 0;
        bool alive = true;
    };
    struct Bolt {
        int lane = 0;
        float x = 0;
        bool live = true;
    };
    struct Puff {
        int lane = 0;
        float x = 0;
        float age = 0;
    };

    void begin();
    void openWave();
    void update();
    void fire();
    void bot();
    void win();
    void lose(const char* why);
    void blip(float freq);
    void serviceAudio();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    float laneY(int lane) const;
    float foeH(int lane) const;
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode pausedFrom_ = Mode::Fight;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE WELL IS STANDING";
    int wave_ = 0;
    int well_ = 8;
    int spawned_ = 0;
    int quota_ = 0;
    int aim_ = 1;
    float t_ = 0;
    float spawnT_ = 0;
    float cool_ = 0;
    float brief_ = 0;
    float shake_ = 0;
    float toneT_ = 0;
    float bucket_ = 0;
    uint32_t rng_ = 0x5177e11u;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
    std::vector<Puff> puffs_;
};

}  // namespace tww
