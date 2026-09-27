// S3 BOCCE GOLD — one end. Gold nearer than the other side counts double.
// Cream nearer than them scores one and does not finish the end.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace boccegold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOCCE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int goldPts() const { return goldPts_; }
    int creamPts() const { return creamPts_; }

private:
    enum class Mode { Title, Aim, Roll, Win, Lose };
    enum class Who { Jack, Gold, Cream, Rival };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        Who who = Who::Jack;
        bool live = false;
    };

    void reset();
    void openEnd();
    void release();
    void coast(float dt);
    void shove();
    bool still() const;
    void score();
    void paint();
    void blit(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void line(int col, int row, const char* s, int pal);
    void lineC(int row, const char* s, int pal);
    int hand() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Who who_ = Who::Jack;
    Ball ball_[4];
    int count_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int goldPts_ = 0;
    int creamPts_ = 0;
    int wait_ = 0;
    float aim_ = 0.f;
    float power_ = 0.4f;
    float powerDir_ = 1.f;
    float clock_ = 0.f;
};

}  // namespace boccegold
