// S3 KART GRASS — take the kart, land on the grass, full stop.
// The clock is the other crew. Missing the grass fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace kartgrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KART GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    float crewLeft() const { return crew_ > 0.f ? crew_ : 0.f; }
    float x() const { return x_; }
    float altitude() const { return y_; }
    float speed() const { return vx_; }
    int phase() const { return phase_; }
    const char* why() const { return why_; }
    // 0 title, 1 the approach, 2 on the grass, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& throttle, float& brake);
    void pilot(float& throttle, float& brake);
    void physics(float throttle, float brake);
    void judge();
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void draw();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool hflip = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    float sx(float wx) const;
    float sy(float h) const;
    bool wheelsOnGrass() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const gs::Mipped* banner_ = nullptr;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool air_ = false;
    bool touchedGrass_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int dustCursor_ = 0;
    float t_ = 0, race_ = 0, crew_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float hold_ = 0;
    float camX_ = 0;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0;
    float dustT_ = 0, shake_ = 0;
    float throttleIn_ = 0;
    float spin_ = 0;
    Puff dust_[12]{};
    char why_[80] = {};
};

}  // namespace kartgrass
