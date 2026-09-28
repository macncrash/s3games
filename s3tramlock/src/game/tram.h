// S3 TRAM LOCK — pass the lock without scraping a gate. The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tramlock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRAM LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float crewLeft() const { return crew_; }
    int gatesClear() const { return cleared_; }
    const char* why() const { return why_; }
    // 0 title, 1 on the approach, 2 in the chamber, 3 both gates clear, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Gate {
        float z;
        float openAt;
        float shutAt;
    };

    void showTitle();
    void startRun();
    void pilot(float& gas, float& brake) const;
    float aperture(int g, float t) const;
    int nextGate() const;
    void physics(float gas, float brake);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void skyRoad();
    bool project(float wz, float wy, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    int chime_ = -1;
    float chimeT_ = 0;
    float beep_ = 0;
    float time_ = 0;
    float crew_ = 0;
    float s_ = 0;
    float v_ = 0;
    float gas_ = 0, brake_ = 0;
    float spark_ = 0;
    int cleared_ = 0;
    bool scraped_ = false;
    Gate gates_[2]{};
};

}  // namespace tramlock
