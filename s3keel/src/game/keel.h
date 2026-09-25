// S3 KEEL — sail a triangle of buoys and come back to the same dock.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace keel {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEEL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int leg() const { return leg_; }
    int waypoint() const { return wp_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }

private:
    enum class Mode { Title, Sail, Pause, Win };

    struct Wake {
        float x, y, life;
    };

    void begin();
    void showTitle();
    void controls(float& steer, float& trim);
    void pilot(float& steer, float& trim);
    void physics(float dt, float steer, float trim);
    void scoreMarks();
    void guide();
    void berth();
    bool inSlip() const;
    void audio(float dt);
    void blip(float freq);
    void chime(int notes);
    void draw();
    void drawHud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int boatFrame() const;
    int sailSide() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool brake_ = false;
    int leg_ = 0;
    int wp_ = 0;
    int tack_ = -1;
    int chime_ = 0;
    int chimeStep_ = 0;
    float t_ = 0;
    float raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, yaw_ = 0;
    float tackTime_ = 10;
    float legTime_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.36f;
    float wakeT_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    std::vector<Wake> wakes_;
};

}  // namespace keel
