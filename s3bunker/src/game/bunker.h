// S3 BUNKER — one concrete room, one blast door.
// Shoot the slit. Brace the bar. Hold until the assault breaks.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace bunker {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUNKER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    // 0 title, 1 the door, 2 the room held, 3 the door opens
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum Kind { WALKER = 0, DUCKER = 1, BREACHER = 2 };

    struct Spawn {
        int frame;
        int kind;
        int lane;
        float x;
    };
    struct Enemy {
        int kind = WALKER;
        int lane = 1;
        int hp = 1;
        int points = 100;
        int age = 0;
        float x = 0.5f;
        float z = 1.f;
        float speed = 0.003f;
        float chew = 0.3f;
        bool alive = true;
    };
    struct Place {
        float x = 0, y = 0, h = 16;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
        int kind = 0;
    };

    void begin();
    void loadWave(int wave);
    void spawnOne(const Spawn& s);
    void input();
    void titleTick();
    void update();
    void endTick();
    void tickMotes();
    bool exposed(const Enemy& e) const;
    Place place(const Enemy& e) const;
    float hitR(const Place& p) const;
    void act(float& ax, float& ay, bool& fire, bool& brace);
    void shoot();
    void shove();
    void kill(Enemy& e);
    void blip(int ch, float freq);
    void puff(float x, float y, int kind);
    void draw();
    void sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);
    void stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bracing_ = false;
    int age_ = 0;
    int score_ = 0;
    int wave_ = 0;
    int waveTime_ = 0;
    int spawnIx_ = 0;
    int calm_ = 0;
    int cool_ = 0;
    int shove_ = 0;
    int flash_ = 0;
    int blipCh_ = 0;
    int blipT_ = 0;
    int wonAge_ = 0;
    float strain_ = 0;
    float shake_ = 0;
    float shx_ = 0, shy_ = 0;
    float aimX_ = 160, aimY_ = 96;
    std::vector<Spawn> spawns_;
    std::vector<Enemy> enemies_;
    Mote motes_[24] = {};
};

}  // namespace bunker
