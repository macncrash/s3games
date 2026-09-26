// S3 YARD RELIEF — hold the yard until the relief bell, then answer it.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace yard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int yard() const { return yard_; }
    int stopped() const { return stopped_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };

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
        float z = 0;
        float age = 0;
        float flash = 0;
        bool alive = true;
    };
    struct Puff {
        float lane = 0;
        float z = 0;
        float t = 0;
    };
    struct Pop {
        float lane = 0;
        float z = 0;
        float t = 0;
        int pts = 0;
    };

    void beginWatch();
    void update(float dt);
    void tickFx(float dt);
    void draw();
    void doDrop();
    void hurt(Foe& f);
    void winWatch();
    void loseWatch(const char* why);
    void project(float worldX, float z, float& sx, float& sy, float& s) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blip(float freq, float vol);
    int nearestLane() const;
    const gs::Mipped& foeImg(const Foe& f) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    bool engineOn_ = false;
    const char* reason_ = "THE WATCH IS OVER";
    int score_ = 0;
    int stopped_ = 0;
    int yard_ = 5;
    int spawnAt_ = 0;
    int fanStep_ = -1;
    float px_ = 0;
    float watch_ = 0;
    float t_ = 0;
    float endT_ = 0;
    float slam_ = 0;
    float dropCd_ = 0;
    float charge_ = 0;
    float chainT_ = 0;
    float chainZ_ = 0;
    int chainLane_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float bellTick_ = 0;
    float fanT_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Puff> puffs_;
    std::vector<Pop> pops_;
};

}  // namespace yard
