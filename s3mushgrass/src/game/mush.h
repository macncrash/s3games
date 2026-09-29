// S3 MUSH GRASS — land the team on the grass and come to a full stop.
// The clock is the other crew.
#pragma once
#include "art.h"
#include "console/system.h"

namespace mushgrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSH GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const;
    float seconds() const { return race_; }
    float clock() const { return clock_; }
    // 0 title, 1 on the snow, 2 on the grass, 3 holding the stop, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Puff {
        float x, y, life;
    };

    void begin();
    void showTitle();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void judge();
    void win();
    void fail(const char* why);
    void chime(int notes);
    void audio(float dt);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawHud();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    int sledFrame(float heading) const;
    bool deepGrass() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool touched_ = false;
    int chimeN_ = 0;
    int chimeStep_ = 0;
    int puffCursor_ = 0;
    float t_ = 0;
    float race_ = 0;
    float clock_ = 0;
    float hold_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float vx_ = 0, vy_ = 0;
    float throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.4f;
    float tone0_ = 0, chimeT_ = 0;
    Puff spray_[12]{};
    char why_[64] = {};
    char line_[48] = {};
};

}  // namespace mushgrass
