// S3 CISTERN RELIEF — at the cistern, hold until the relief bell.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace cistern {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERN RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };
    enum Kind { RAIDER = 0, RUNNER = 1, PLATE = 2 };

    struct Spawn {
        float t;
        int kind;
        int bear;
    };
    struct Foe {
        int kind = 0;
        int bear = 0;
        int hp = 1;
        int points = 0;
        float r = 1.f;
        float speed = 0.2f;
        int age = 0;
        bool alive = true;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void place(int bear, float rad, float& x, float& y) const;
    float eta(const Foe& f) const;
    void shoot();
    void kill(Foe& f);
    void winWatch();
    void loseWatch(const char* why);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    const char* reason_ = "THE WATCH FAILED";
    int score_ = 0;
    int face_ = 0;
    int spawnAt_ = 0;
    int flash_ = 0;
    int fanStep_ = -1;
    float watch_ = 0;
    float rope_ = 0;
    float cool_ = 0;
    float shake_ = 0;
    float bellTick_ = 0;
    float modeT_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
};

}  // namespace cistern
