// S3 COMBINE — one pass down the wheat. The header stays full.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "art.h"

namespace combine {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 COMBINE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int metres() const { return int(s_); }

private:
    enum class Mode { Title, Drive, Pause, Won, Lost };

    struct Prop {
        float s, x;
        int kind;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void toTitle();
    void beginDrive();
    void updateDrive(float dt);
    void win();
    void lose();
    void draw();
    void sound();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false, bool shadow = false);
    float botSteer() const;
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float s_ = 0;
    float x_ = 0;
    float latV_ = 0;
    float speed_ = 0;
    float cruise_ = 0;
    float header_ = 100;
    float supply_ = 1;
    float view_ = 0;
    float beep_ = 0;
    float shake_ = 0;
    float shakePx_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    uint32_t rng_ = 0xC0B1u;
    std::vector<Prop> props_;
    Mote motes_[32]{};
};

}  // namespace combine
