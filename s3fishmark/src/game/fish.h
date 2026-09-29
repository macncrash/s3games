// S3 FISH MARK — fish until the catch covers the painted mark. That ends it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace fishmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FISH MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    // 0 title, 1 the cast, 2 the fight, 3 on the board, 4 the mark is finished
    int marker() const;

private:
    enum class Mode { Title, Cast, Wait, Fight, Measure, Win };

    void begin();
    void controls();
    void pilot();
    void stepCast();
    void stepWait();
    void stepFight();
    void stepMeasure();
    void snapLine();
    void win();
    void audio();
    void draw();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool holdA_ = false;
    bool tapA_ = false;
    bool left_ = false;
    bool right_ = false;
    bool start_ = false;
    bool charging_ = false;
    float t_ = 0;
    float race_ = 0;
    float power_ = 0;
    float cast_ = 0.55f;
    float phase_ = 0;
    float biteAt_ = 1.6f;
    float fish_ = 0;
    float rod_ = 0;
    float progress_ = 0;
    float strain_ = 0;
    float lay_ = 0;
    float tone_ = 0;
    float toneV_ = 0;
    char note_[48] = {};
    char report_[160] = {};
};

}  // namespace fishmark
