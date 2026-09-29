// S3 HEADER LOCK — one rally leg. In the header, pass the lock
// without scraping a gate. Missing the end fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace headerlock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADER LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 header, 2 through the lock, 3 leg after the lock, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Gate {
        float z;
        float gap;
        bool crossed;
        bool clean;
    };

    void begin();
    void pilot(float& thrust, float& steer, float& brake);
    void step(float thrust, float steer, float brake);
    void fail(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    void project(float wz, float wx, float wy, float& sx, float& sy, float& scale, int& fog) const;
    void mark(const gs::Mipped& m, float wz, float wx, float wy, float worldH, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float bend(float z) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float z_ = 0;
    float x_ = 0;
    float speed_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    bool lockOpen_ = false;
    Gate gates_[3] = {};
};

}  // namespace headerlock
