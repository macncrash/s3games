// S3 CHEF SEVEN — plate in the gold. First cook to seven.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace chefseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHEF SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };

    struct Pan {
        int dish = -1;
        float heat = 0;
        bool dinged = false;
    };

    void beginMatch();
    void toTitle();
    void humanAct(float dt);
    void botAct();
    void nudge(int dir);
    bool plate(int pan);
    void advance(float dt);
    void tryDrop();
    void rivalStep(float dt);
    void finishIf();
    void chime(int ch, float freq, float vol, float hold);
    void audio(float dt);

    void backdrop();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void solid(float x, float y, float w, float h, int pal);
    void bar(float x, float y, float w, float heat);
    float demoHeat(int i) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int them_ = 0;
    int sel_ = 1;
    int holdDir_ = 0;
    int nextDish_ = 0;
    float hold_ = 0;
    float dropCd_ = 0;
    float rival_ = 0;
    float anim_ = 0;
    float reach_ = 0;
    float shake_ = 0;
    float fanT_ = 0;
    float chefX_ = 160;
    float chime_[3] = {};
    Pan pans_[PANS];
};

}  // namespace chefseven
