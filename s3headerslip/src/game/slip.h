// S3 HEADER SLIP — tack on the header, then berth before the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace headerslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADER SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_; }
    float seconds() const { return race_; }
    float crewShort() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    // 0 title, 1 the beat, 2 in the slip, 3 holding the berth, 4 over
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };
    enum class Phase { Lay, Header1, Lift, Header2, Final };

    void showTitle();
    void begin();
    void tick(float steer, float throttle, bool tack);
    void pilot(float& steer, float& throttle, bool& tack);
    void human(float& steer, float& throttle, bool& tack);
    void succeed();
    void fail(const char* why);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    int hullFrame(float h) const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Lay;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool headerLive_ = false;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float throttle_ = 0;
    float race_ = 0, call_ = 0, lift_ = 0, hold_ = 0, penalty_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 5.f;
    float toneT_ = 0, t_ = 0;
    int scrapes_ = 0;
    char why_[48] = "running";
};

}  // namespace headerslip
