// S3 BUS — six stops. The doors open only in the box.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace bus {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return line_; }
    int cleared() const { return next_; }
    float seconds() const { return time_; }

private:
    enum class Mode { Title, Drive, Pause, Win, Fail };

    struct In {
        float steer = 0, throttle = 0, brake = 0;
        bool doors = false;
        bool doorsEdge = false;
    };
    struct Obs {
        float cx, cy, hw, hh;
        int kind;
    };

    void showTitle();
    void begin();
    void layout();
    In readInput();
    In human();
    In pilot() const;
    void physics(const In& in, float dt);
    void collide();
    void separate(float cx, float cy, float hw, float hh);
    void bump();
    void tryDoors(const In& in);
    void openDoors();
    void deny();
    void say(const char* s);
    void finish(bool win, const char* why);
    void chime(bool fanfare);
    void audio(float dt);
    void follow(float dt);
    void lights();
    void draw();
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, bool shadow);
    void world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip);
    void toScreen(float wx, float wy, float& sx, float& sy) const;
    bool contained(float cx, float cy, float hw, float hh) const;
    bool boxOverlap(float cx, float cy, float hw, float hh) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int next_ = 0;
    int score_ = 0;
    int boarded_ = 0;
    int hits_ = 0;
    int doorSide_ = -1;
    int song_ = -1;
    int songN_ = 0;
    float songT_ = 0;
    float buzzT_ = 0;
    float time_ = 0;
    float dwell_ = 0;
    float hurt_ = 0;
    float anim_ = 0;
    float x_ = 160, y_ = 130, speed_ = 0, steer_ = 0, camY_ = 130;
    float msgT_ = 0;
    const char* why_ = nullptr;
    char msg_[48] = {};
    char line_[160] = {};
    std::vector<Obs> obs_;
};

}  // namespace bus
