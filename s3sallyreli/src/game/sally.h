// S3 SALLY RELIEF — one sally. Hold the ground outside the gate until the relief bell.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace sally {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the sally, 2 the bell is ringing, 3 the sally has ended
    int marker() const;

private:
    enum class Mode { Title, Sally, Pause, Victory, Over };
    enum Kind { RAIDER = 0, RUNNER = 1, SHIELD = 2 };

    struct Spawn {
        float t;
        int kind;
        int lane;
    };
    struct Foe {
        int kind = 0;
        int lane = 0;
        int hp = 1;
        int points = 0;
        float x = 0;
        bool on = true;
    };
    struct Puff {
        float x, y, t, r;
    };
    struct Pop {
        float x, y, t;
        int pts;
    };

    void beginSally();
    void update(float dt);
    void draw();
    void strike();
    void shoot();
    void fell(Foe& f);
    void winSally();
    void loseSally(const char* why);
    int soonest(int lane) const;
    void bot(bool& up, bool& down, bool& pike, bool& shot);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal);
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
    const char* reason_ = "THE SALLY FAILED";
    int score_ = 0;
    int lane_ = 1;
    int hp_ = 4;
    int spawnAt_ = 0;
    int fanStep_ = -1;
    int flash_ = 0;
    float x_ = 118.f;
    float watch_ = 0;
    float pikeCd_ = 0;
    float shotCd_ = 0;
    float laneCd_ = 0;
    float shake_ = 0;
    float bellTick_ = 0;
    float modeT_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Puff> puffs_;
    std::vector<Pop> pops_;
};

}  // namespace sally
