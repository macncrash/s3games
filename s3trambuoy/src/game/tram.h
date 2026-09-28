// S3 TRAMBUOY — round the harbour buoys and tie up at the same dock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tram {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRAMBUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int buoys() const { return next_ > 3 ? 3 : next_; }
    float clock() const { return clock_; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    void begin();
    void controls(float& thrust, float& rudder);
    void pilot(float& thrust, float& rudder);
    void physics(float dt, float thrust, float rudder);
    void logic(float dt);
    void draw();
    void drawHud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int hullFrame() const;
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int next_ = 0;
    float clock_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float camX_ = 0, camY_ = 0;
    float bell_ = 0;
    float wakeT_ = 0;
    struct Wake {
        float x, y, life;
    };
    Wake wakes_[28] = {};
    int wakeN_ = 0;
};

}  // namespace tram
