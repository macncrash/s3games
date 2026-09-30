// S3 HELIBUOY — round the buoys and set down on the same dock.
// The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace buoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELIBUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float left() const { return left_; }
    int marks() const { return got_; }
    int lives() const { return lives_; }
    const char* why() const { return why_ && why_[0] ? why_ : "hung"; }

private:
    enum class Mode { Title, Play, Win, Fail };

    struct Mark {
        float x, y;
        bool done;
    };

    void begin();
    void human(float& sx, float& sy);
    void pilot(float& sx, float& sy);
    void physics(float dt, float sx, float sy);
    void roundBuoy(float dt);
    void tryDock(float dt);
    void crash();
    void win();
    void clockOut();
    void draw();
    void place(const gs::Mipped& m, float wx, float wy, float worldW, float worldH, int pal, bool flip, float ax, float ay);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mark marks_[3]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    int got_ = 0;
    int lives_ = 3;
    float t_ = 0;
    float left_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float lap_ = 0;
    float lastAng_ = 0;
    bool haveAng_ = false;
    float settle_ = 0;
    float invuln_ = 0;
    float heading_ = 0;
    float camX_ = 0, camY_ = 0;
};

}  // namespace buoy
