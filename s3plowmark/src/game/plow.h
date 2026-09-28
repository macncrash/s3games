// S3 PLOWMARK — the plow has one job: set down on the mark.
// The blade starts up. The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace plow {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PLOWMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    // 0 title, 1 rolling, 2 blade over the paint, 3 holding the set, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Win, Fail };

    void beginRun();
    void update(float dt, bool drive, bool brake, bool down);
    void win();
    void fail(const char* why);
    void draw();
    void sky();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    float sx(float wx, float par = 1.f) const;
    void bot(bool& drive, bool& brake, bool& down) const;
    void blip(float freq);
    void audio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float x_ = 0, v_ = 0;
    float blade_ = 0;
    float hold_ = 0;
    float sit_ = 0;
    float race_ = 0;
    float t_ = 0;
    float spray_ = 0;
    float tone_ = 0, toneT_ = 0;
    bool wasDown_ = false;
    const char* why_ = nullptr;
    uint64_t tick_ = 0;
};

}  // namespace plow
