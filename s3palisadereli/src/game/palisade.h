// One palisade. Hold the stakes until the relief bell, then answer it.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace palisade {

class Game : public gs::Cart {
public:
    const char* title() const override { return "PALISADE RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the wall, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };
    enum Kind { CLIMB = 0, RUNNER = 1, SHIELD = 2 };

    struct Spawn {
        float t;
        int post;
        int kind;
    };
    struct Foe {
        int kind = 0;
        int post = 0;
        int hp = 1;
        int points = 0;
        float climb = 0;
        float speed = 0.14f;
        int age = 0;
        bool alive = true;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void thrust();
    void fell(Foe& f);
    void winWatch();
    void loseWatch(const char* why);
    float postX(float post) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
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
    const char* reason_ = "THE WALL FAILED";
    int score_ = 0;
    int want_ = 2;
    int flash_ = 0;
    int fanStep_ = -1;
    float px_ = 2;
    float watch_ = 0;
    float rope_ = 0;
    float cool_ = 0;
    float shake_ = 0;
    float bellTick_ = 0;
    float modeT_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    int spawnAt_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
};

}  // namespace palisade
