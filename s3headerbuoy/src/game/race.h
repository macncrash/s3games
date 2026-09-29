// S3 HEADER BUOY — take the header, round the buoys, return to the same dock.
// The clock is the other crew.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace headerbuoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADER BUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float raceTime() const { return raceT_; }
    float crewTime() const { return crew_; }
    int buoys() const { return cleared_; }

private:
    enum class Mode { Title, Race, Win, Lose };

    struct Buoy {
        float x, y;
        float sweep = 0;
        float lastAng = 0;
        bool inRing = false;
        bool done = false;
        const char* name = "";
    };
    struct Wp {
        float x, y;
    };

    void beginRace();
    void update(float dt);
    void draw();
    void steerBoat(float dt, float want);
    void layHeading(float tx, float ty, float& want) const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip = false);
    float windFrom() const;
    bool headerNow() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool gun_ = false;
    bool leftDock_ = false;
    int cleared_ = 0;
    int wp_ = 0;
    int menu_ = 0;
    float t_ = 0;
    float raceT_ = 0;
    float crew_ = 64.0f;
    float x_ = 0, y_ = 0, heading_ = 1.5707963f, speed_ = 0;
    float camX_ = 0, camY_ = 0;
    float tack_ = 0;
    float tackFrom_ = 0, tackTo_ = 0;
    float headerFlash_ = 0;
    float boost_ = 0;
    float msgT_ = 0;
    std::string msg_;
    std::vector<Buoy> buoys_;
    std::vector<Wp> wps_;
};

}  // namespace headerbuoy
