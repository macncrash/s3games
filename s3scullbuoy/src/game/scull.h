// S3 SCULL — take the single scull, round the buoys, and beat the other crew home.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace scull {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCULL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float you() const { return you_; }
    float crew() const { return crewTime_; }
    int buoy() const { return next_; }
    float x() const { return x_; }
    float y() const { return y_; }

private:
    enum class Mode { Title, Race, Pause, Win, Fail };

    struct Foam {
        float x, y, life;
    };

    void begin();
    void controls(float& thrust, float& steer);
    void pilot(float& thrust, float& steer);
    void physics(float dt, float thrust, float steer);
    void marks(float dt);
    void rivalAt(float t, float& x, float& y, float& heading) const;
    void audio(float dt, float thrust);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int shellFrame(float heading) const;
    void blip(float freq);
    void fanfare();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int next_ = 0;
    int fanStep_ = -1;
    float you_ = 0;
    float crewTime_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float stroke_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.7f;
    float tone_ = 0;
    float pathLen_ = 1;
    uint32_t rng_ = 1;
    std::vector<Foam> foam_;
};

}  // namespace scull
