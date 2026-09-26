// S3 GATE RELIEF — hold the gate until the relief bell, then answer it.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace gate {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int gate() const { return gate_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };

    struct Foe {
        int id = 0;
        int kind = 0;
        int lane = 0;
        float z = 0;
        float speed = 0;
        float age = 0;
        float flash = 0;
        int hp = 1;
        int points = 0;
        bool alive = true;
    };
    struct Bolt {
        int lane = 0;
        float z = 0;
        float prev = 0;
    };
    struct Pop {
        float x = 0;
        float z = 0;
        float t = 0;
        int pts = 0;
    };
    struct Spawn {
        float t;
        int kind;
        int lane;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void project(float worldX, float z, float& sx, float& sy, float& s) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void tone(float freq, float vol);
    void winWatch();
    void loseWatch(const char* why);
    void hurt(Foe& f);
    int indexOf(int id) const;
    int nearestLane() const;
    float eta(const Foe& f) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    bool bellAnnounced_ = false;
    bool droneOn_ = false;
    const char* reason_ = "WATCH OVER";
    int gate_ = 8;
    int score_ = 0;
    int nextId_ = 1;
    int focus_ = -1;
    int spawnAt_ = 0;
    float t_ = 0;
    float watch_ = 0;
    float px_ = 0;
    float rope_ = 0;
    float meleeCd_ = 0;
    float boltCd_ = 0;
    float swing_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float endT_ = 0;
    float bellTick_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
    std::vector<Pop> pops_;
};

}  // namespace gate
