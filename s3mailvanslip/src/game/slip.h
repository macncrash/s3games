// S3 MAILVAN SLIP — berth the mail van in the quay slip before the tide turns.
// Missing the end of the slip fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace mailvanslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAILVAN SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    float tideLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return hdg_; }
    float speed() const { return spd_; }
    bool inSlip() const { return inSlip_; }
    bool inBerth() const { return inBerth_; }
    const char* why() const { return why_; }
    // 0 title, 1 fairway, 2 in the slip, 3 holding the berth, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void pilot(float& steer, float& thr);
    void human(float& steer, float& thr);
    void step(float steer, float thr);
    void fail(const char* why);
    void win();
    void draw();
    void spr(const gs::Mipped& m, float wx, float wy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inSlip_ = false;
    bool inBerth_ = false;
    float t_ = 0;
    float race_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0, spd_ = 0, thr_ = 0;
    float hold_ = 0;
    float cam_ = 0;
    float tone_ = 0;
    float tick_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    char why_[48] = {};
};

}  // namespace mailvanslip
