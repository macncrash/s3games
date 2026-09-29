// S3 HEADER PLAT — one rally leg. Stop level with the platform.
// Missing the end fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace headerplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADER PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

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
    bool level() const;

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
    float hold_ = 0;
    float bad_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace headerplat
