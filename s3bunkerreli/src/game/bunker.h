// S3 BUNKER RELIEF — you have the bunker. Hold until the relief bell.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace bunker {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUNKER RELIEF"; }
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
    enum Kind { WALKER = 0, SPRINTER = 1, PLATE = 2 };

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
        float z = 1.2f;
        float speed = 0.25f;
        int age = 0;
        bool alive = true;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void place(const Foe& f, float& x, float& y, float& h) const;
    float eta(const Foe& f) const;
    void shoot();
    void kill(Foe& f);
    void winWatch();
    void loseWatch(const char* why);
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
    const char* reason_ = "THE WATCH FAILED";
    int score_ = 0;
    int lane_ = 0;
    int spawnAt_ = 0;
    int flash_ = 0;
    int fanStep_ = -1;
    float watch_ = 0;
    float aim_ = 0;
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

}  // namespace bunker
