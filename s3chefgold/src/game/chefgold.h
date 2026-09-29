// S3 CHEF GOLD — plate the gold. Only the gold counts double.
#pragma once
#include "console/system.h"
#include "game/pictures.h"

namespace chefgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHEF GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int line() const { return kLine; }
    bool goldOut() const { return goldOut_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 service, 2 first gold, 3 rush, 4 left on a gold double
    int marker() const;

    static constexpr int kLine = 12;

private:
    enum class Mode { Title, Play, Pause, Win, Lose };

    struct Pan {
        int order = -1;
        float heat = 0;
        bool gold = false;
    };

    void enterTitle();
    void beginService();
    void humanAct(float dt);
    void botAct();
    void nudge(int dir);
    bool plate(int pan);
    void advance(float dt);
    void tryDrop();
    void leaveIf();
    void fail(const char* why);
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
    bool goldOut_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int next_ = 0;
    int sel_ = 1;
    int holdDir_ = 0;
    float hold_ = 0;
    float dropCd_ = 0;
    float anim_ = 0;
    float chefX_ = 160;
    Pan pans_[3];
    char reason_[40] = {};
};

}  // namespace chefgold
