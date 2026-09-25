// S3 BARON — a Western Front sortie on the S3-16.
// Waves of scouts, balloons and two-seaters, then a red triplane.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace baron {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BARON"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int lives() const { return lives_; }
    // 0 menu, 1 combat, 2 wave banner, 3 the ace is up, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Menu, Help, Brief, Fly, Clear, Dead, Over, Victory, Pause };
    enum class Kind { Scout, Balloon, Bomber, Ace };

    struct Spawn {
        float t;
        Kind kind;
        float x, y;
    };
    struct Enemy {
        Kind kind;
        float x, y, z, hp, age, shoot, home, oy, amp, freq, homing, speed, flash, radius, height;
        int points;
        bool leak;
        bool leaked;
    };
    struct Shot {
        float x, y, z, vx, vy;
        bool hostile;
    };
    struct Popup {
        float x, y, t;
        int pts;
    };
    struct Prop {
        float x, z;
        int kind;
    };

    void bootSortie(bool practice);
    void prepareWave();
    void update(float dt);
    void draw();
    void banners();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    float rnd();
    void blip(bool high);
    void explode();
    void fanfare();
    void addKill(int pts, float sx, float sy);
    void stick(float& sx, float& sy, bool& fire, bool& roll);
    void hurt(int dmg);
    const char* waveName() const;
    const char* waveOrder() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Fly;
    bool bot_ = false;
    bool practice_ = false;
    bool over_ = false;
    bool won_ = false;
    bool aceUp_ = false;
    int menu_ = 0;
    int wave_ = 0;
    int score_ = 0;
    int lives_ = 4;
    int hull_ = 100;
    float t_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float scroll_ = 0;
    float fireCd_ = 0, roll_ = 0, rollCd_ = 0, invuln_ = 0, shake_ = 0;
    float beep_ = 0;
    float lastKill_ = -10;
    float practiceSpawn_ = 1;
    uint32_t rng_ = 0x51A7u;
    uint16_t skyTop_ = 0, skyHor_ = 0, skyFog_ = 0;
    std::vector<Spawn> spawns_;
    std::vector<Enemy> enemies_;
    std::vector<Shot> shots_;
    std::vector<Popup> pops_;
    std::vector<Prop> props_;
    bool engineOn_ = false;
    int fanStep_ = -1;
    float fanT_ = 0;
};

}  // namespace baron
