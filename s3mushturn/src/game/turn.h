// S3 MUSHTURN — three corners in the mush. Tip and the leg is gone.
// Missing the end fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace mushturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSHTURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return turnsMade_; }

private:
    enum class Mode { Title, Run, Miss, Win, Lose };

    struct Corner {
        float z0, z1;
        float dir;  // +1 lean right, -1 lean left
        float credit;
        bool made;
    };

    void paintSky();
    void paintTrail();
    void paintWorld();
    void paintTeam();
    void glyphText(const char* s, int x, int y, int scale, int pal);
    void project(float wx, float wz, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip);
    void bake();
    float curve(float z) const;
    float heading(float z) const;
    void begin();
    void restartLeg();
    void mush(float dt);
    void botInput(float& leanWant, float& throttle) const;
    void miss(const char* why);
    void blip(float freq);
    int cornerAt(float z) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int turnsMade_ = 0;
    int lives_ = 3;
    float t_ = 0;
    float x_ = 0;
    float pz_ = 0;
    float speed_ = 9.0f;
    float lean_ = 0;
    float stride_ = 0;
    float hold_ = 0;
    float flash_ = 0;
    float blip_ = 0;
    float wrong_ = 0;
    float flatLean_ = 0;
    const char* note_ = "";
    float curveTab_[201] = {};
    Corner corners_[3]{};
};

}  // namespace mushturn
