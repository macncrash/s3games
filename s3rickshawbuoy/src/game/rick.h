// S3 RICKSHAW BUOY — round the buoys to port and stop in the same dock.
#pragma once
#include "art.h"
#include "console/system.h"

namespace rick {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW BUOY"; }
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
    float speed() const { return surge_; }
    float seconds() const { return raceTime_; }
    const char* why() const { return why_; }
    int marker() const;

private:
    enum class Mode { Title, Ride, Pause, Win, Fail };

    struct Puff {
        float x, y, life;
    };

    void begin();
    void showTitle();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void scoreMarks();
    void guide();
    bool inSlip() const;
    bool inWrong() const;
    void finish();
    void miss(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio(float dt);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawHud();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    int shawFrame() const;
    void puff(float x, float y);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool armed_ = false;
    bool wrong_ = false;
    bool gate_ = false;
    int leg_ = 0;
    int wp_ = 0;
    int fare_ = 0;
    int chimeN_ = 0;
    int chimeStep_ = 0;
    int dustCursor_ = 0;
    float t_ = 0;
    float raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, surge_ = 0, yaw_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.55f;
    float throttle_ = 0;
    float bellT_ = 0, bellF_ = 440.f;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0, dustT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    const char* why_ = "";
    Puff dust_[16]{};
};

}  // namespace rick
