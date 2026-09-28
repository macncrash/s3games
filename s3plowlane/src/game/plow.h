// S3 PLOWLANE — take the plow and stay in the lane for the whole leg.
// The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace plow {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PLOWLANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float legTime() const { return time_; }
    float crewTime() const { return crew_; }

private:
    enum class Mode { Title, Drive, Over };

    void resetLeg();
    void update(float dt);
    void draw();
    void sky();
    void road();
    void world();
    void hud();
    void hudText(int col, int row, const char* s, int pal);
    void blit(const gs::Mipped& m, float cx, float footY, float h, int pal, bool flip, int fog);
    void ui(const gs::Image& im, float x, float y, int pal);
    float laneAt(float z) const;
    bool inLane() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool engineOn_ = false;
    float z_ = 0;
    float x_ = 0;
    float latV_ = 0;
    float time_ = 0;
    float off_ = 0;
    float crew_ = 46.f;
    int hor_ = 92;
};

}  // namespace plow
