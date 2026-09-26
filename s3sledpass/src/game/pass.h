// S3 SLED PASS — one dogsled. Clear the pass before the storm clock dies.
// Reaching the arch after the clock has already gone is a fail.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sledpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const;
    float seconds() const { return raceTime_; }
    float clockLeft() const;
    float meters() const { return playerZ_; }
    float x() const { return playerX_; }
    float speed() const { return speed_; }
    // 0 title, 1 the mush call, 2 the pass, 3 the storm closing, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Go, Run, Pause, Result };
    enum class End { None, Clear, Storm, Buried };
    enum class Kind { Pine, Stake, Cabin, Boulder, MushGate, PassGate };

    struct Prop {
        float z, x, h;
        Kind kind;
    };
    struct Hazard {
        float z, x, r;
        int rock;  // 0 snow drift, 1 boulder in the lane
        bool hit;
    };
    struct Draw {
        float z;
        gs::Sprite s;
    };
    struct Puff {
        float x, y, vx, vy, life, sc;
    };
    struct Flake {
        float x, y, sp, sc;
    };
    struct Proj {
        bool ok;
        float x, y, hw, fog, z;
    };

    void tune();
    void buildCourse();
    void resetRun();
    void launch();
    void end(End e);
    void human(float& steer, bool& mush, bool& brake);
    void bot(float& steer, bool& mush, bool& brake) const;
    void physics(float dt);
    void hazards();
    void audio(float dt);
    void flakes(float dt);
    void fadePuffs(float dt);
    void spawnPuff(float x, float y, float vx, float vy);
    void draw();
    void sky();
    void road();
    void world();
    void team();
    void gate(float z, const gs::Mipped& banner);
    void hud();
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void queue(float z, const gs::Sprite& s);
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow = false);
    void stretch(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog, float z);
    void ui(const gs::Image& img, float x, float y, int pal);
    Proj project(float worldZ, float roadX) const;
    float shiftAt(float z) const;
    float kappa(float z) const;
    float feed(float k) const;
    float lookX() const;
    float camZ() const { return playerZ_; }
    float stormAmt() const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Prop> props_;
    std::vector<Hazard> haz_;
    std::vector<Draw> draws_;
    std::vector<float> shiftU_;
    Puff puff_[16]{};
    Flake flake_[22]{};

    Mode mode_ = Mode::Title;
    End end_ = End::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool mushing_ = false;
    bool braking_ = false;
    int hor_ = 76;
    int shakeX_ = 0;
    int shakeY_ = 0;
    int puffN_ = 0;
    int beepSec_ = -1;
    int chime_ = 0;

    float modeTime_ = 0;
    float raceTime_ = 0;
    float playerZ_ = 0;
    float playerX_ = 0;
    float latV_ = 0;
    float speed_ = 0;
    float yaw_ = 0;
    float steerSm_ = 0;
    float shake_ = 0;
    float chimeT_ = 0;
};

}  // namespace sledpass
