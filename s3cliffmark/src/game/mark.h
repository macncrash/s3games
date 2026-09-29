// S3 CLIFF MARK — set down on the mark. Missing the end fails the leg.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cliffmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFF MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceT_; }
    const char* why() const { return why_.c_str(); }
    float along() const { return z_; }
    float lateral() const { return u_; }
    float speed() const { return speed_; }
    float nose() const { return noseErr_; }
    int phase() const { return phase_; }
    // 0 title, 1 on the leg, 2 nose on the mark, 3 holding the set, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& gas, float& brake, float& steer, float& rev);
    void pilot(float& gas, float& brake, float& steer, float& rev);
    void physics(float dt, float gas, float brake, float steer, float rev);
    bool onMark() const;
    float noseZ() const;
    const char* stopWhy() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    bool project(float lat, float z, float& sx, float& sy, float& sh) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool announced_ = false;
    int phase_ = 0;
    int chime_ = 0;
    float chimeT_ = 0;
    float tone_ = 0;
    float t_ = 0;
    float raceT_ = 0;
    float clock_ = 0;
    float z_ = 0;
    float u_ = 0;
    float speed_ = 0;
    float hold_ = 0;
    float outT_ = 0;
    float noseErr_ = 0;
    float shake_ = 0;
    std::string why_;
};

}  // namespace cliffmark
