// S3 SCULL SLIP — berth the scull in the slip before the tide turns.
// The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace scullslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCULL SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return you_; }
    float tideLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float speed() const { return speed_; }
    bool inSlip() const { return inSlip_; }
    bool inEnd() const { return inEnd_; }
    const char* why() const { return why_; }
    // 0 title, 1 reach, 2 in the slip, 3 holding the berth, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Row, Pause, Win, Fail };

    struct Foam {
        float x, y, life;
    };

    void begin();
    void controls(float& row, float& steer);
    void pilot(float& row, float& steer);
    void physics(float dt, float row, float steer);
    void audio(float dt, float row);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int shellFrame() const;
    void blip(float freq);
    void finish(bool win, const char* why);
    bool sheltered() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inSlip_ = false;
    bool inEnd_ = false;
    int fanStep_ = -1;
    float you_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float stroke_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.2f;
    float tone_ = 0;
    Foam foam_[14]{};
    int foamN_ = 0;
    char why_[48] = {};
};

}  // namespace scullslip
