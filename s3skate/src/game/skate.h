// S3 SKATE — land the line. A fall zeros the run.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace skate {

class Game : public gs::Cart {
public:
    static constexpr int kTricks = 6;

    const char* title() const override { return "S3 SKATE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int run() const { return score_; }
    int falls() const { return falls_; }
    int line() const;
    const char* fail() const { return fail_; }

private:
    enum class Mode { Title, Run, Pause, Fall, Win };

    void beginRun(bool keepFalls);
    void physics();
    void bail(const char* why);
    void scoreTrick(int i);
    void airStep();
    void groundStep();
    void grindStep();
    bool botTap() const;
    float stick() const;
    int deckAt(float x) const;
    void audio();
    void blip(float freq, float vol, int frames);
    void draw();
    void backdrop();
    void world();
    void rider();
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip = false, int fog = 0,
               bool shadow = false);
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false, int fog = 0);
    float screenX(float wx) const;
    float screenY(float wy) const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    const char* coach() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = true;
    bool grinding_ = false;
    bool grinded_ = false;
    bool manual_ = false;
    bool did_[kTricks] = {};
    int score_ = 0;
    int falls_ = 0;
    int hold_ = 0;
    int tick_ = 0;
    int fallT_ = 0;
    int blip_ = 0;
    int fanT_ = 0;
    int fanStep_ = -1;
    int grindFrames_ = 0;
    int shake_ = 0;
    float t_ = 0;
    float x_ = 40;
    float y_ = 0;
    float vy_ = 0;
    float angle_ = 0;
    float camX_ = 40;
    float sayT_ = 0;
    const char* fail_ = "";
    char say_[24] = {};
};

}  // namespace skate
