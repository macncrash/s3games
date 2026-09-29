// S3 BOARDMARK — one night desk. The gold lamp is the mark.
// Seating the cord on it finishes the mark. That finished mark ends the board.
// The operator then leaves. Other lamps are not the job.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace boardmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOARDMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool left() const { return left_; }
    int markLine() const { return MARK_LINE; }

private:
    enum class Mode { Title, Brief, Play, Leave, Fail };

    struct Call {
        bool on = false;
        bool clearing = false;
        bool mark = false;
        int trunk = 0;
        int line = 0;
        float life = 0;
        float maxLife = 0;
    };

    void begin();
    void play(float dt);
    void steer(float dt);
    void botAct();
    void seat();
    void tick(float dt);
    void dropMark();
    void audio(float dt);
    void blip(bool high);
    void click();
    void buzz();
    int irnd(int n);
    Call* liveTrunk(int i);
    const Call* markCall() const;

    void draw();
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);
    void sprBox(const gs::Mipped& m, float x, float y, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool left_ = false;
    int bank_ = 0;
    int row_ = 0;
    int held_ = -1;
    float t_ = 0;
    float rep_ = 0;
    float hold_ = 0;
    float opX_ = 40;
    float ringT_ = 0;
    float beep_ = 0;
    uint32_t rng_ = 0xB0ADu;
    Call calls_[JACKS];
};

}  // namespace boardmark
