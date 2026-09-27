// S3 CAB TURN — three city turns, upright, before the other crew's clock.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace cabturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAB TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceTime_; }
    float crewLeft() const;
    int turns() const { return turns_; }
    float lean() const { return lean_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    const char* why() const { return why_; }
    // 0 title, 1 first turn, 2 second, 3 third, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Node {
        float x, y;
        int turn;  // 0 straight, 1..3 the corner this sample belongs to
    };

    void buildPath();
    void begin();
    void showTitle();
    void startRun();
    void human(float& steer, float& throttle, float& brake);
    void pilot(float& steer, float& throttle, float& brake);
    void physics(float dt, float steer, float throttle, float brake);
    void succeed();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio(float dt);
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool hflip = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0, bool hflip = false);
    int cabFrame(float hdg) const;
    float roadDist(float x, float y) const;
    int nearest(int from, float x, float y, int window) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Node> path_;
    int apex_[4] = {};
    int finishI_ = 0;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int turns_ = 0;
    int prog_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    float t_ = 0, raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float lean_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 4.6f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, tickT_ = 0;
    char why_[48] = {};
};

}  // namespace cabturn
