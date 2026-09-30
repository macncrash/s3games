// S3 BUSBUOY — round the buoys to port and return to the same dock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace buoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUSBUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return line_; }
    int rounded() const { return rounded_; }
    float seconds() const { return time_; }

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct In {
        float steer = 0;
        float throttle = 0;
        float brake = 0;
    };
    struct Wp {
        float x, y;
        int mark;  // buoy this gate belongs to, or -1 for the dock
    };

    void begin();
    void course();
    In readIn();
    In human();
    In pilot() const;
    void physics(const In& in);
    void gates();
    void hitBuoy(int i);
    void finish(bool win);
    void audio();
    void draw();
    void sprite(const gs::Image& img, float wx, float wy, int w, int h, int pal, bool flip);
    void text(int x, int y, const char* s, int pal);
    void center(int y, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int rounded_ = 0;
    int wp_ = 0;
    int markWp_[3] = {};
    int hold_ = 0;
    int song_ = 0;
    float time_ = 0;
    float limit_ = 110;
    float x_ = 440, y_ = 620, heading_ = 0, speed_ = 0;
    float camX_ = 440, camY_ = 560;
    float bump_ = 0;
    float flash_ = 0;
    char line_[160] = {};
    char hud_[48] = {};
    Wp wps_[16] = {};
    int nwp_ = 0;
};

}  // namespace buoy
