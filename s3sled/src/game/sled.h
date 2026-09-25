// S3 SLED — one snow run. The other sled is the clock.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sled {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    // 0 title, 1 countdown, 2 chasing, 3 ahead of the clock, 4 finished
    int marker() const;
    double raceSeconds() const { return raceTime_; }

private:
    enum class Mode { Title, Count, Run, Result };
    enum class Kind { Rock, Tree, Flag, Drop, Finish };
    enum class Banner { None, Passed, Lost };

    struct Thing {
        float z, x, r;
        Kind kind;
        int var;
        bool solid;
        bool hit;
    };
    struct Flake {
        float x, y, sp, sc;
    };
    struct Puff {
        float x, y, vx, vy, life, sc;
    };
    struct Draw {
        float z;
        gs::Sprite s;
    };
    struct Proj {
        bool ok;
        float x, y, hw, fog, z;
    };

    void tune();
    void buildCourse();
    void resetRun();
    void beginCount();
    void launch();
    void human(float& steer, bool& tuck, bool& brake);
    void bot(float& steer, bool& tuck, bool& brake);
    void race(float dt);
    void collide(float prevZ);
    void bump(float keep);
    void finishRace();
    void audio(float dt);
    void draw();
    void sky();
    void road();
    void world();
    void hud();
    void hudText(int col, int row, const std::string& s, int pal);
    void queue(float z, const gs::Sprite& s);
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow = false);
    void ui(const gs::Image& img, float x, float y);
    Proj project(float worldZ, float roadX) const;
    float shiftAt(float z) const;
    float kappa(float z) const;
    bool ice(float z) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Thing> things_;
    std::vector<Draw> draws_;
    Flake flakes_[20]{};
    Puff puffs_[28]{};
    float shiftU_[421]{};

    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool lead_ = false;
    bool tock_ = false;
    Banner banner_ = Banner::None;
    int crashes_ = 0;
    int puffN_ = 0;
    int hor_ = 106;
    int shakeX_ = 0;
    int shakeY_ = 0;
    char report_[160] = {};

    float modeTime_ = 0;
    float raceTime_ = 0;
    float playerZ_ = 0;
    float playerX_ = 0;
    float latV_ = 0;
    float speed_ = 0;
    float rivalZ_ = 0;
    float rivalSpd_ = 0;
    float yaw_ = 0;
    float steerSm_ = 0;
    float stun_ = 0;
    float shake_ = 0;
    float bankCd_ = 0;
    float bannerT_ = 0;
    float tickAcc_ = 0;
    float tickVol_ = 0;
    float tickFreq_ = 330;
    float fanT_ = 0;
    float margin_ = 0;
    bool tucking_ = false;
    bool braking_ = false;
};

}  // namespace sled
