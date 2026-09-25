// S3 OUTPOST — flares show them. Last until the dawn mark.
#pragma once

#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace outpost {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OUTPOST"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int down() const { return down_; }
    int post() const { return post_; }
    int score() const { return score_; }
    // 0 title, 1 the watch, 2 the dawn mark, 3 the wire falls
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };
    enum class Kind { Skulk, Runner, Brute };

    struct Spawn {
        float t, ang, rad;
        Kind kind;
    };
    struct Foe {
        Kind kind = Kind::Skulk;
        float x = 0, y = 0, speed = 16, bob = 0;
        int hp = 1, points = 100, dmg = 28;
        float hit = 14;
        bool alive = true;
    };
    struct Flare {
        float ox = 0, oy = 0, lx = 0, ly = 0, t = 0, flight = 0.2f, burn = 3.3f;
        bool popped = false;
    };
    struct Bolt {
        float x = 0, y = 0, px = 0, py = 0, vx = 0, vy = 0, life = 0;
    };
    struct Spark {
        float x = 0, y = 0, vx = 0, vy = 0, t = 0;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
        bool bad = false;
    };

    void paintGround();
    void beginWatch();
    void update(float dt);
    void think(float& mx, float& my, bool& fire, bool& toss, bool& lamp);
    void shoot();
    void tossFlare();
    void strike(Foe& e);
    void kill(Foe& e);
    void beginWin();
    void beginLoss();
    void quiet();
    void blip(int ch, float freq, float vol, float hold);
    void fanfare(bool dawn);
    float lightAt(float x, float y) const;
    bool revealed(float x, float y) const;
    bool landingCovers(const Foe& e) const;
    void flareGround(const Flare& f, float& x, float& y) const;
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int down_ = 0;
    int post_ = 100;
    int score_ = 0;
    int stock_ = 8;
    int spawnIx_ = 0;
    float t_ = 0;
    float face_ = -1.5708f;
    float px_ = 160, py_ = 112;
    float fireCd_ = 0, tossCd_ = 0, lamp_ = 0, lampCd_ = 0, regen_ = 0;
    float muzzle_ = 0, shake_ = 0, hurt_ = 0, ping_ = 0.4f;
    float beep_[3] = {};
    std::vector<Foe> foes_;
    std::vector<Flare> flares_;
    std::vector<Bolt> bolts_;
    std::vector<Spark> sparks_;
    std::vector<Pop> pops_;
};

}  // namespace outpost
