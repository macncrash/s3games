// S3 TRAM PASS — take the tram and clear the pass.
// The storm clock is the other crew. A wrong throw at the points derails.
// Reaching the arch after their clock has already gone also fails the leg.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace trampass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRAM PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const;
    float seconds() const { return raceTime_; }
    float crewLeft() const;
    float x() const { return throwSm_; }
    float meters() const { return playerZ_; }
    float speed() const { return speed_; }
    // 0 title, 1 the yard, 2 the pass, 3 the other crew closing, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Go, Run, Pause, Result };
    enum class End { None, Clear, Storm, Derail };

    struct Point {
        float z;
        int side;  // -1 left rail, +1 right rail
        bool done;
    };
    struct Prop {
        float z, x, h;
        int kind;  // 0 pine, 1 mast, 2 rock, 3 yard gate, 4 pass gate, 5 point sign
    };
    struct Draw {
        float z;
        gs::Sprite s;
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
    void human(float& lever, bool& gas, bool& brake);
    void bot(float& lever, bool& gas, bool& brake) const;
    void physics(float dt);
    void audio(float dt);
    void flakes(float dt);
    void draw();
    void sky();
    void road();
    void world();
    void tram();
    void rival();
    void gate(float z, const gs::Mipped& banner);
    void hud();
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void queue(float z, const gs::Sprite& s);
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z,
              bool shadow = false);
    void ui(const gs::Image& img, float x, float y, int pal);
    Proj project(float worldZ, float roadX) const;
    float shiftAt(float z) const;
    float kappa(float z) const;
    float lookX() const { return 0.f; }
    float camZ() const { return playerZ_; }
    float stormAmt() const;
    const Point* nextPoint() const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Point> points_;
    std::vector<Prop> props_;
    std::vector<Draw> draws_;
    std::vector<float> shiftU_;
    Flake flake_[20]{};

    Mode mode_ = Mode::Title;
    End end_ = End::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool gassing_ = false;
    bool braking_ = false;
    int hor_ = 76;
    int shakeX_ = 0;
    int shakeY_ = 0;
    int beepSec_ = -1;
    int chime_ = 0;

    float modeTime_ = 0;
    float raceTime_ = 0;
    float playerZ_ = 0;
    float rivalZ_ = 0;
    float speed_ = 0;
    float yaw_ = 0;
    float throwSm_ = 0;
    float shake_ = 0;
    float chimeT_ = 0;
};

}  // namespace trampass
