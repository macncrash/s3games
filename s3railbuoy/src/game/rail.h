// S3 RAIL BUOY — one cab on a harbour loop.
// Round the three buoys, then stop in the same dock. The far pier is not the end.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace railbuoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAIL BUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_; }
    int leg() const { return 1; }
    int buoys() const { return wp_; }
    float along() const { return s_; }
    float speed() const { return speed_; }
    float seconds() const { return raceTime_; }
    // 0 title, 1 out on the rail, 2 rounding, 3 the return, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Win, Fail };

    void begin();
    void controls(float& thr, float& brk);
    void pilot(float& thr, float& brk);
    void physics(float dt, float thr, float brk);
    void checkPass(float prev);
    bool inDock(float s) const;
    bool inFar(float s) const;
    void finish();
    void fail(const char* why);
    void audio(float dt);
    void blip(float freq);
    void draw();
    void sky();
    void sprite(const gs::Image& img, float x, float y, int pal, int sw = 0, int sh = 0);
    void text(float x, float y, const char* s, int pal);
    void textC(float y, const char* s, int pal);
    void poseAt(float s, float& x, float& y, float& hx, float& hy) const;
    float length() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    int wp_ = 0;
    int chimeStep_ = -1;
    float t_ = 0;
    float raceTime_ = 0;
    float s_ = 0;
    float speed_ = 0;
    float stopT_ = 0;
    float farT_ = 0;
    float tone_ = 0;
    float chimeT_ = 0;
    char why_[48] = {};
};

}  // namespace railbuoy
