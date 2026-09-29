// S3 CHEFMARK — a short chef.
// One steak. Plate it while the heat sits on the gold mark. That finished mark ends it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace chefmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHEFMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int score() const { return score_; }
    const char* fail() const { return fail_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };

    void begin();
    void toTitle();
    void plate();
    void advance(float dt);
    void botAct();
    void chime(int ch, float freq, float vol, float hold);
    void audio(float dt);
    void lights();

    void backdrop();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    int score_ = 0;
    float heat_ = 0;
    float t_ = 0;
    float fan_ = 0;
    float chime_[3] = {};
    bool dinged_ = false;
    char fail_[40] = {};
};

}  // namespace chefmark
