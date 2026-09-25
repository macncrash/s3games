// S3 EIGHT — one rack. Call the pocket.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace eight {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EIGHT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int shots() const { return shots_; }
    int left() const { return upCount(true); }
    const char* pocketName() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Stroke, Place, Roll, Pause, Win, Lose };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool down = false;
        bool fell = false;
        int pocket = -1;
    };
    struct Plan {
        bool ok = false;
        bool place = false;
        float x = 0, y = 0, angle = 0, power = 0.4f, dot = 1;
        int pocket = 0;
        int ball = 1;
    };

    void resetMatch();
    void begin();
    void rack();
    void shoot();
    void physics(float dt);
    void resolve();
    void respot(int n);
    void dropCue(float x, float y);
    void sink(Ball& b, int pocket);
    bool anyMoving(float v) const;
    bool jaw();
    void forceStop();
    void botAct();
    void human();
    void draw();
    void backdrop();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void stamp(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void stampM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void aimAid();
    void cueDraw();
    void fanfare();
    void winRack(int pocket);
    void loseRack(const char* why);
    Plan choose();
    Plan bestMake(bool allowPlace);
    Plan bestScatter();
    bool placeOnLine(int ball, float dirx, float diry, float& ox, float& oy) const;
    bool pathClear(float x0, float y0, float x1, float y1, int ignoreA, int ignoreB) const;
    bool crossesPocket(float x0, float y0, float x1, float y1, int allow) const;
    bool inTable(float x, float y) const;
    bool overlaps(float x, float y, int ignore) const;
    bool freePoint(float x, float y, int ignore) const;
    int upCount(bool includeEight) const;
    int pocketAlong(float x, float y, float dx, float dy) const;
    void toneOff();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Stroke;
    Ball ball_[16]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool opening_ = true;
    bool commit_ = false;
    bool pocketSnd_ = false;
    bool clack_ = false;
    int called_ = 1;
    int eightPocket_ = 1;
    int commitPocket_ = 1;
    int score_ = 0;
    int shots_ = 0;
    int stall_ = 0;
    int othersBesideEight_ = 14;
    int fanStep_ = -1;
    float aim_ = 0;
    float power_ = 0.62f;
    float commitAim_ = 0;
    float commitPower_ = 0.5f;
    float t_ = 0;
    float rollT_ = 0;
    float sayT_ = 0;
    float fanT_ = 0;
    const char* say_ = "ONE RACK";
    const char* reason_ = "TIME";
};

}  // namespace eight
