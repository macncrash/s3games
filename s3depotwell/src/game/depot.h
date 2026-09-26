// S3 DEPOT WELL — at the depot, keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace depotwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int wave() const { return wave_; }
    int breaches() const { return breaches_; }
    int held() const { return held_; }
    int courses() const { return courses_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 3 the well stands, 4 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Pause, Victory, Over };

    struct Spawn {
        float t = 0;
        int lane = 0;
        int kind = 0;
        float speed = 40;
    };
    struct Wagon {
        int lane = 0;
        int kind = 0;
        float nose = 0;
        float speed = 40;
        float hold = 0;
        float anim = 0;
        float flash = 0;
        bool alive = true;
        bool pinned = false;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };
    struct Puff {
        float x = 0, y = 0, t = 0;
    };

    void begin();
    void toTitle();
    void startWave();
    void buildScript(int wave);
    void updatePlay(float dt);
    void updateBanner(float dt);
    void bot(float& steer, bool& brake);
    void human(float& steer, bool& brake);
    void moveShunter(float steer, bool brake, float dt);
    void updateWagons(float dt);
    void couple(Wagon& w);
    void hitWell(Wagon& w);
    void win();
    void lose();
    void fadeFx(float dt);
    void blip(float freq, float vol);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip);
    void shadowAt(float x, float y, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    int urgent() const;
    const char* waveName(int w) const;
    const char* sidingName(int lane) const;
    float halfOf(int kind) const;
    float bodyH(int kind) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool upLatch_ = false;
    bool dnLatch_ = false;
    bool braking_ = false;
    bool droneWatch_ = false;
    const char* reason_ = "UNFINISHED";
    int wave_ = 0;
    int nextWave_ = 0;
    int courses_ = 3;
    int breaches_ = 0;
    int held_ = 0;
    int score_ = 0;
    int spawnAt_ = 0;
    int laneTarget_ = 1;
    int fanStep_ = -1;
    float lanePos_ = 1.f;
    float x_ = 172.f;
    float speed_ = 0.f;
    float t_ = 0.f;
    float tWave_ = 0.f;
    float bannerT_ = 0.f;
    float shake_ = 0.f;
    float fanT_ = 0.f;
    float camX_ = 0.f;
    float camY_ = 0.f;
    std::vector<Spawn> script_;
    std::vector<Wagon> wagons_;
    std::vector<Pop> pops_;
    std::vector<Puff> puffs_;
};

}  // namespace depotwell
