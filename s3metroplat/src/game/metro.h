// S3 METRO PLAT — the metro has one job: stop level with the platform.
// The car door has to sit on the yellow gap line and the wheels have to be still.
// Stop short, run the end wall, or let the service clock expire, and the stop fails.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace metroplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METRO PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float door() const { return pos_; }
    float speed() const { return speed_; }
    const char* why() const { return why_; }
    // 0 title, 1 in the tunnel, 2 beside the platform, 3 holding level, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Roll, Pause, Won, Lost };

    struct Spark {
        float x = 0, age = 0;
    };

    void startRun();
    void botControls(float& thr, float& brk) const;
    void updateRoll(float dt);
    void fail(const char* why);
    void winRun();
    void bell();
    void audio(float dt);
    void tunnel();
    void drawPoster();
    void drawRun();
    void drawBanner();
    void hud();
    void draw();
    void hudText(int col, int row, const char* s, int pal);
    void hudCenter(int row, const char* s, int pal);
    void blit(const gs::Image& img, float x, float y, int pal, int w = 0, int h = 0, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool motorsOn_ = false;
    bool wasBraking_ = false;
    bool held_ = false;
    int fanStep_ = -1;
    float pos_ = 0;
    float speed_ = 0;
    float time_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float bell_ = 0;
    float blip_ = 0;
    float sparkT_ = 0;
    float fanT_ = 0;
    float thr_ = 0;
    float brk_ = 0;
    char why_[48] = {};
    Spark sparks_[6]{};
};

}  // namespace metroplat
