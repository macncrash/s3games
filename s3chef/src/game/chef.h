// S3 CHEF — ten tickets on the heat. Plate each one while the bar is gold.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace chef {

constexpr int PANS = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int plated() const { return plated_; }
    const char* fail() const { return fail_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };

    struct Pan {
        int ticket = -1;
        float heat = 0;
        bool dinged = false;
    };
    struct Pop {
        float t = 0;
        int col = 0;
        int row = 0;
        int pal = 0;
        char text[16] = {};
    };

    void beginService();
    void toTitle();
    void humanAct(float dt);
    void botAct();
    void nudge(int dir);
    bool plate(int pan);
    void advance(float dt);
    void tryDrop();
    void pop(int pan, const char* text, int pal);
    void tickPops(float dt);
    void chime(int ch, float freq, float vol, float hold);
    void audio(float dt);
    void lights();
    void glide(float dt);

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
    int score_ = 0;
    int plated_ = 0;
    int streak_ = 0;
    int next_ = 0;
    int sel_ = 1;
    int burned_ = -1;
    int holdDir_ = 0;
    float hold_ = 0;
    float dropCd_ = 0;
    float t_ = 0;
    float anim_ = 0;
    float reach_ = 0;
    float shake_ = 0;
    float fanT_ = 0;
    float chefX_ = 160;
    float chime_[3] = {};
    bool done_[TICKETS] = {};
    Pan pans_[PANS];
    std::vector<Pop> pops_;
    char fail_[32] = {};
};

}  // namespace chef
