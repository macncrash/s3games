// S3 MUSHPASS — clear the pass before the storm clock.
// Missing the end of a leg fails that leg. The clock does not wait.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace mushpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSHPASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int legs() const { return legsDone_; }
    float storm() const { return clock_; }

private:
    enum class Mode { Title, Run, Miss, Win, Lose };

    struct Leg {
        float startZ;
        float endZ;
        float doneZ;
    };

    void paintSky();
    void paintSnow();
    void paintWorld();
    void paintTeam();
    void glyphText(const char* s, int x, int y, int scale, int pal);
    void project(float wx, float wz, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip);
    void begin();
    void startLeg();
    void mush(float dt);
    void botStick(float& steer, float& power) const;
    void miss(const char* why);
    void stormOut();
    void blip(float freq);
    float curve(float z) const;
    float halfW(float z) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool endTaken_ = false;
    int leg_ = 0;
    int legsDone_ = 0;
    int lives_ = 3;
    float t_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float pz_ = 0;
    float stride_ = 0;
    float hold_ = 0;
    float flash_ = 0;
    float clock_ = 0;
    float scrape_ = 0;
    const char* note_ = "";
    Leg legs_[3]{};
};

}  // namespace mushpass
