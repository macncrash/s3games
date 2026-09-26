// S3 RIDGE RELIEF — hold the ridge until the relief bell, then haul the rope.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace reli {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stopped() const { return stopped_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Brief, Watch, Pause, Victory, Over };

    struct Spawn {
        float t;
        int kind;
        float u;
    };
    struct Foe {
        int kind;
        float u, z, age, flash;
        int hp, points;
        bool on;
    };
    struct Bolt {
        float u, z, prev;
        bool on;
    };
    struct Puff {
        float u, z, t;
    };
    struct Pop {
        float x, y, t;
        int pts;
    };
    struct Spot {
        float x, y, h;
        int fog;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void hurt(Foe& f);
    void looseBolt();
    void pike();
    void winWatch();
    void loseWatch(const char* why);
    void tickAudio();
    bool atBell() const;
    Spot spot(float u, float z, float base) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    const gs::Mipped& foeImg(int kind, int frame) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    const char* reason_ = "THE WATCH RAN OUT";
    int score_ = 0;
    int stopped_ = 0;
    int spawnAt_ = 0;
    int fanStep_ = -1;
    float u_ = 0;
    float face_ = 1;
    float watch_ = 0;
    float modeT_ = 0;
    float rope_ = 0;
    float fireCd_ = 0;
    float pikeCd_ = 0;
    float swing_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float bellTick_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
    std::vector<Puff> puffs_;
    std::vector<Pop> pops_;
};

}  // namespace reli
