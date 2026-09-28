// S3 MILLDAWN — one mill. Keep the flares lit until dawn.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace mill {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILLDAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int flaresLit() const;
    // 0 title, 1 early watch, 2 late watch, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Victory, Defeat };

    struct Flare {
        float x, y;
        float fuel;
        float gust;
    };

    void begin();
    void update(float dt);
    void botThink(float& ax, float& ay, bool& stoke);
    void tryStoke();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void tone(float f, float v, float hold);
    bool allLit() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int score_ = 0;
    int stokes_ = 0;
    float px_ = 160;
    float py_ = 168;
    int face_ = 1;
    float stokeT_ = 0;
    float cool_ = 0;
    float clock_ = 0;
    float t_ = 0;
    float gustEvery_ = 0;
    int gustAt_ = 0;
    float beep_ = 0;
    float noteT_ = 0;
    int noteI_ = 0;
    bool fan_ = false;
    Flare flares_[4]{};
};

}  // namespace mill
