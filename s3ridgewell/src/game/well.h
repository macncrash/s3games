// S3 RIDGE WELL — one ridge. Keep the well standing through three waves.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace rwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE WELL"; }
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
    enum class Mode { Title, Play, Banner, Pause, Victory, Fail };

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
        float z = 0;
        float speed = 40;
        float bash = 0;
        float anim = 0;
        float flash = 0;
        bool alive = true;
    };
    struct Puff {
        float z = 0, u = 0, age = 0, life = 0.4f;
    };
    struct Prop {
        float z = 0, u = 0, h = 1;
        int kind = 0;
    };
    struct Spot {
        float x = 0, y = 0, h = 0;
        int fog = 0;
        bool ok = false;
    };

    void bootTitle();
    void beginRun();
    void startWave();
    void buildScript(int wave);
    void buildProps();
    void updatePlay(float dt);
    void updateBanner(float dt);
    void aim(float dt);
    int bestFoe() const;
    int coveredLane() const;
    void thrust(int lane);
    void kill(Foe& f);
    void breach(Foe& f);
    void lose();
    void win();
    void fadeFx(float dt);
    void blip(float freq);
    void serviceAudio();
    void draw();
    void layRoad();
    Spot spot(float u, float z, float base) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    int horizon() const;
    const char* waveName(int w) const;
    const char* hint() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bracing_ = false;
    const char* reason_ = "UNFINISHED";
    int wave_ = 0;
    int nextWave_ = 0;
    int stones_ = 4;
    int through_ = 0;
    int score_ = 0;
    int target_ = 1;
    int spawnAt_ = 0;
    int holdDir_ = 0;
    int fanStep_ = -1;
    int hor_ = 72;
    float t_ = 0;
    float tWave_ = 0;
    float u_ = 0;
    float bannerT_ = 0;
    float thrustCd_ = 0;
    float thrustT_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float holdT_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace rwell
