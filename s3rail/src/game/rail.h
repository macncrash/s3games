// S3 RAIL — one cab, one line, four stop boxes.
// A late arrival, or running past the box, costs the run.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace rail {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAIL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopsMade() const { return made_; }
    int lateCount() const { return late_; }
    const char* lineName() const { return "WEST LINE"; }
    const char* endNote() const { return note_; }

private:
    enum class Mode { Title, Roll, Dwell, Pause, Won, Lost };

    struct Puff {
        float x = 0, age = 0;
    };

    void startRun();
    void updateTitle();
    void updatePause();
    void updateEnd(float dt);
    void updateDwell(float dt);
    void updateRoll(float dt);
    void arrive();
    void fail(const char* why);
    void winRun();
    void botControls(float& thr, float& brk) const;
    void audio(float dt);
    void whistle();
    void blip(float freq);

    void draw();
    void sky();
    void drawPoster();
    void drawRun();
    void drawBanner();
    void hud();
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
    bool boardPassed_ = false;
    bool wasBraking_ = false;
    int leg_ = 0;
    int made_ = 0;
    int late_ = 0;
    int fanStep_ = -1;
    float pos_ = 80.f;
    float speed_ = 0.f;
    float legTime_ = 0.f;
    float dwell_ = 0.f;
    float spare_ = 0.f;
    float t_ = 0.f;
    float whistle_ = 0.f;
    float blip_ = 0.f;
    float puffT_ = 0.f;
    float fanT_ = 0.f;
    float thr_ = 0.f;
    float brk_ = 0.f;
    const char* note_ = "";
    Puff puffs_[8]{};
};

}  // namespace rail
