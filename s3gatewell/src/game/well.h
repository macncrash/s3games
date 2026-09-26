// S3 GATE WELL — you have the gate. Keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace well {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stones() const { return stones_; }
    int through() const { return through_; }
    int wave() const { return wave_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 3 the well stands, 4 the well fell
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Pause, Victory, Over };

    struct Spawn {
        float t = 0;
        int lane = 0;
        int kind = 0;
        float speed = 40;
    };
    struct Foe {
        int lane = 0;
        int kind = 0;
        int hp = 1;
        float x = 0;
        float speed = 40;
        float bash = 0;
        float anim = 0;
        float flash = 0;
        bool alive = true;
        bool leaked = false;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };
    struct Puff {
        float x = 0, y = 0, t = 0;
    };

    void beginRun();
    void startWave();
    void buildScript(int wave);
    void updatePlay(float dt);
    void updateBanner(float dt);
    void aim(float dt);
    int bestFoe() const;
    void strike(int lane);
    void kill(Foe& f);
    void lose();
    void win();
    void fadeFx(float dt);
    void blip(float freq, float vol);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip);
    void shadowAt(float x, float y, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    const char* waveName(int w) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bracing_ = false;
    bool droneOn_ = false;
    bool droneWatch_ = false;
    const char* reason_ = "UNFINISHED";
    int wave_ = 0;
    int nextWave_ = 0;
    int stones_ = 4;
    int through_ = 0;
    int score_ = 0;
    int gateTarget_ = 1;
    int spawnAt_ = 0;
    int fanStep_ = -1;
    float gateLane_ = 1;
    float t_ = 0;
    float tWave_ = 0;
    float bannerT_ = 0;
    float slamCd_ = 0;
    float slamT_ = 0;
    float shake_ = 0;
    float endT_ = 0;
    float fanT_ = 0;
    float camX_ = 0;
    float camY_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Pop> pops_;
    std::vector<Puff> puffs_;
};

}  // namespace well
