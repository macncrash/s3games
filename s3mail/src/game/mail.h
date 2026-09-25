// S3 MAIL — the box gets the paper. Don't miss the turn.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/pictures.h"

namespace mail {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAIL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return clock_; }
    int boxesGot() const;
    int boxCount() const { return 4; }
    int turnsMade() const;
    int turnCount() const { return 3; }
    const char* fail() const { return fail_ != nullptr ? fail_ : ""; }

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Box {
        float s = 0;
        int side = 1;
        bool got = false;
    };
    struct Turn {
        float s = 0;
        int side = -1;
        bool made = false;
        bool resolved = false;
    };
    struct Prop {
        float s = 0, x = 0;
        int kind = 0;
        int variant = 0;
    };
    struct Toss {
        bool live = false;
        float t = 0, dur = 0;
        float landS = 0, landX = 0;
        int side = 1;
    };
    struct Puff {
        float s = 0, x = 0, t = 0;
    };
    struct Pop {
        float s = 0, x = 0, t = 0;
        const char* text = "";
        int pal = 0;
    };
    struct Ahead {
        float h = 0, x = 0;
    };

    void layout();
    void resetRun();
    void enterTitle();
    void clearProgress();
    int nextBox() const;
    int nextTurn() const;
    int boxesLeft() const;
    float leadDist() const;
    float curvature(float s) const;
    bool throwWindow(int bi) const;
    bool tossHits(int bi) const;
    void throwPaper();
    void updatePaper(float dt);
    void checkPass();
    void checkTurns();
    void checkFinish();
    void win();
    void fail(const char* why);
    void addPop(float s, float x, const char* text, int pal);
    void addPuff(float s, float x);
    float botSteer() const;
    bool botToss() const;
    void updateRun(float steer, bool fast, bool easy, bool toss);
    void audio(float dt);

    void cacheAhead();
    void roadPoint(float dist, float& h, float& x) const;
    bool project(float wx, float ws, float& sx, float& sy, float& scale, int& fog, float minDist) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false, bool shadow = false);
    void hud(int col, int row, const char* text, int pal);
    void hudC(int row, const char* text, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void drawRoad();
    void drawWorld();
    void drawBike();
    void drawToss(float u, float landS, float landX);
    void drawCall();
    void drawHud();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* fail_ = "";
    Box boxes_[4];
    Turn turns_[3];
    std::vector<Prop> props_;
    Toss paper_{};
    Puff puffs_[10]{};
    int puffN_ = 0;
    Pop pops_[8]{};
    int popN_ = 0;
    float s_ = 0, x_ = 0, vx_ = 0, v_ = 0;
    float steer_ = 0;
    float clock_ = 0, t_ = 0, shake_ = 0;
    float camX_ = 0, camY_ = 0;
    int papers_ = 6;
    int fanStep_ = -1;
    float fanT_ = 0;
    float chain_ = 0;

    static constexpr int AHEAD_N = 180;
    Ahead ahead_[AHEAD_N + 1]{};
};

}  // namespace mail
