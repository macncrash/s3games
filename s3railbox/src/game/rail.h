// S3 RAILBOX — take the rail and stop inside the box.
// The clock beside the line is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace railbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAILBOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float nose() const { return pos_; }
    float speed() const { return speed_; }
    const char* why() const { return why_; }
    // 0 title, 1 on the rail, 2 nose in the box, 3 holding the stop, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Roll, Pause, Won, Lost };

    struct Puff {
        float x = 0, age = 0;
    };

    void startRun();
    void botControls(float& thr, float& brk) const;
    void updateRoll(float dt);
    void arrive();
    void fail(const char* why);
    void winRun();
    void whistle();
    void blip(float freq);
    void audio(float dt);
    void sky();
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
    bool engineOn_ = false;
    bool wasBraking_ = false;
    bool held_ = false;
    int fanStep_ = -1;
    float pos_ = 0;
    float speed_ = 0;
    float time_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float whistle_ = 0;
    float blip_ = 0;
    float puffT_ = 0;
    float fanT_ = 0;
    float thr_ = 0;
    float brk_ = 0;
    char why_[48] = {};
    Puff puffs_[8]{};
};

}  // namespace railbox
