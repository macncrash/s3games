// S3 METROBOOM — take the metro and deliver the drive to the boom.
// The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace metro {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METROBOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float clock() const { return clock_; }
    float crewLeft() const { return crewLeft_; }

private:
    enum class Mode { Title, Run, Over };

    void showTitle();
    void begin();
    void physics(float dt, bool gas, bool brake);
    void judge();
    void pilot(bool& gas, bool& brake) const;
    void audio(float dt);
    void lights();
    void draw();
    void blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false);
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float cam() const;
    bool inBoom() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool seated_ = false;
    int strikes_ = 0;
    int song_ = -1;
    float songT_ = 0;
    float clock_ = 0;
    float crewLeft_ = 0;
    float nose_ = 0;
    float speed_ = 0;
    float anim_ = 0;
    float msgT_ = 0;
    const char* why_ = nullptr;
    bool crossed_[2] = {};
};

}  // namespace metro
