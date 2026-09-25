// S3 TUG — walk a freighter into the slip. A piling fails the job.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace tug {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUG"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool piled() const { return piled_; }
    float seconds() const { return job_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const;

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    struct Wake {
        float x, y, life;
    };

    void begin();
    void controls(float& thrust, float& rudder, float& tug);
    void pilot(float& thrust, float& rudder, float& tug);
    void physics(float dt, float thrust, float rudder, float tug);
    void corners(float xs[4], float ys[4]) const;
    bool hitPile(int& which) const;
    bool boxed() const;
    bool aligned() const;
    void resolveWalls();
    void audio(float dt, float thrust, float tug);
    void blip(float freq);
    void horn();
    void chime();
    void draw();
    void drawHud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int shipFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool piled_ = false;
    int pile_ = -1;
    float job_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float surge_ = 0, sway_ = 0, yaw_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1;
    float shake_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0;
    float wakeT_ = 0;
    int chime_ = 0, chimeStep_ = 0;
    float thrustIn_ = 0, tugIn_ = 0;
    std::vector<Wake> wakes_;
};

}  // namespace tug
