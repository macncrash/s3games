// S3 HEADER GRASS — take the header, land on the grass, come to a full stop.
// The clock is the other crew.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace headergrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADER GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float raceTime() const { return raceT_; }
    float crewTime() const { return crew_; }
    bool stopped() const { return stopped_; }
    bool tookHeader() const { return headerTaken_; }

private:
    enum class Mode { Title, Race, Win, Lose };
    struct Wp {
        float x, y;
    };

    void beginRace();
    void update(float dt);
    void draw();
    void steerBoat(float dt, float want);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip = false);
    float windFrom() const;
    bool headerNow() const;
    bool onGrass() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool gun_ = false;
    bool headerTaken_ = false;
    bool stopped_ = false;
    int wp_ = 0;
    int botPhase_ = 0;
    float t_ = 0;
    float raceT_ = 0;
    float crew_ = 36.0f;
    float x_ = 0, y_ = 40, heading_ = 1.5707963f, speed_ = 0;
    float camX_ = 0, camY_ = 80;
    float tack_ = 0;
    float tackFrom_ = 0, tackTo_ = 0;
    float boost_ = 0;
    float still_ = 0;
    float msgT_ = 0;
    std::string msg_;
    std::vector<Wp> wps_;
};

}  // namespace headergrass
