// S3 REARGUARD — walk the column back. The fight is on the road behind it.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace rearguard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 REARGUARD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int column() const { return column_; }
    int wounds() const { return wounds_; }
    float victoryAge() const { return victoryT_; }
    // 0 title, 1 the lane, 2 the last rise, 3 the gate
    int marker() const;

private:
    enum class Mode { Title, Fight, Pause, Victory, Fail };

    struct Cue {
        float t;
        int kind;
        float lane;
    };
    struct Foe {
        int kind = 0;
        float lane = 0, x = 0, z = 0, hp = 1, speed = 2, age = 0, flash = 0, hit = 0.22f, tall = 1;
        int score = 100;
        bool alive = true;
    };
    struct Shot {
        float x = 0, z = 0;
        bool alive = true;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };
    struct Prop {
        float x = 0, z = 0;
        int kind = 0;
    };
    struct Proj {
        float x = 0, y = 0, t = 0;
        int fog = 0;
    };

    void beginMarch();
    void updateTitle(float dt);
    void updateFight(float dt);
    void reachGate();
    void breakColumn();
    void spawnCue(const Cue& c);
    void hurt();
    void leak();
    void note(int pts, float wx, float wz);
    void stick(float& sx, float& sy, bool& fire);
    void drum();
    void blip(int ch, float freq, float vol, float dur);
    void fanfare();
    void audio(float dt);
    void driftProps(float dt);
    float rnd();
    float bend(float z) const;
    Proj project(float x, float z) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false,
             bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void drawFoe(int kind, float x, float z, float tall, int frame, float flash);
    void drawColumn();
    void drawWorld();
    void draw();
    const char* legName() const;
    int legNow() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool droneOn_ = false;
    int column_ = 8;
    int wounds_ = 3;
    int score_ = 0;
    int leg_ = 0;
    int stepPar_ = 0;
    int cue_ = 0;
    int fanStep_ = -1;
    float clock_ = 0;
    float march_ = 0;
    float victoryT_ = 0;
    float failT_ = 0;
    float banner_ = 0;
    float px_ = 0, pz_ = 1.42f, vx_ = 0;
    float fireCd_ = 0, fireFlash_ = 0, invuln_ = 0, shake_ = 0, shx_ = 0;
    float stepAcc_ = 0;
    float psgT_[3] = {};
    float fanT_ = 0;
    float leakFlash_ = 0;
    uint32_t rng_ = 0x4E1Au;
    std::string bannerText_;
    std::vector<Cue> cues_;
    std::vector<Foe> foes_;
    std::vector<Shot> shots_;
    std::vector<Pop> pops_;
    std::vector<Prop> props_;
};

}  // namespace rearguard
