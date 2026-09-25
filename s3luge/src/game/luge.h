// S3 LUGE — one ice chute. Stay off the walls.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    float meters() const { return playerZ_; }
    // 0 title, 1 countdown, 2 on the line, 3 near a wall, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Drop, Run, Result };
    enum class Verdict { None, Clean, Wall, Slow };
    enum class Kind { Gantry, Drop, Finish };

    struct Prop {
        float z;
        Kind kind;
    };
    struct Draw {
        float z;
        gs::Sprite s;
    };
    struct Puff {
        float x, y, vx, vy, life, sc;
    };
    struct Mote {
        float x, y, sp, sc;
    };
    struct Proj {
        bool ok;
        float x, y, hw, fog, z;
    };

    void clearRun();
    void beginDrop();
    void launch();
    void buildChute();
    void tune();
    void human(float& steer, bool& tuck, bool& brake);
    void bot(float& steer, bool& tuck, bool& brake);
    void physics(float dt);
    void end(bool win, Verdict v);
    void audio(float dt);
    void motes(float dt);
    void fadePuffs(float dt);
    void spawnPuff(float x, float y, float vx, float vy);
    void draw();
    void sky();
    void road();
    void world();
    void hud();
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void queue(float z, const gs::Sprite& s);
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow = false);
    void stretch(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog, float z);
    void ui(const gs::Image& img, float x, float y);
    void gate(float z, const gs::Mipped& banner, int postPal);
    void gantry(float z);
    Proj project(float worldZ, float roadX) const;
    float shiftAt(float z) const;
    float kappa(float z) const;
    float feed(float k) const;
    float camZ() const;
    float lookX() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Prop> props_;
    std::vector<Draw> draws_;
    std::vector<float> shiftU_;
    Puff puff_[20]{};
    Mote mote_[18]{};

    Mode mode_ = Mode::Title;
    Verdict verdict_ = Verdict::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool tucking_ = false;
    bool braking_ = false;
    int hor_ = 80;
    int shakeX_ = 0;
    int shakeY_ = 0;
    int puffN_ = 0;
    char report_[160] = {};

    float modeTime_ = 0;
    float raceTime_ = 0;
    float playerZ_ = 0;
    float playerX_ = 0;
    float latV_ = 0;
    float speed_ = 0;
    float yaw_ = 0;
    float steerSm_ = 0;
    float shake_ = 0;
    float scroll_ = 0;
    float chime_ = 0;
    float fanT_ = 0;
};

}  // namespace luge
