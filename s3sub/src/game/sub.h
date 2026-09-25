// S3 SUB — a dark channel. Don't scrape the bottom.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sub {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const;
    int scrapes() const { return scrapes_; }
    double seconds() const { return run_; }
    double x() const { return x_; }
    float lowest() const { return minClear_; }
    const char* note() const { return note_; }

private:
    enum class Mode { Title, Dive, Pause, Clear, Breach };

    struct Bit {
        float x, y, vx, vy, life;
        int kind;
    };

    void measure();
    void showTitle();
    void begin();
    void controls(float& thrust, float& throttle);
    void pilot(float& thrust, float& throttle);
    void physics(float thrust, float throttle);
    void collide();
    void mote(float x, float y, float vx, float vy, int kind);
    void blow();
    void audio();
    void draw();
    void drawSub();
    void drawLife();
    void drawGate();
    void drawRocks();
    void drawHud();
    void label(const gs::Mipped& m, float cx, float top, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool flip);
    void sprRect(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void gapNow(float& bot, float& top) const;
    void look(float ahead, float& bot, float& top) const;
    float lightAt(float wx, float wy) const;
    int fogAt(float wx, float wy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool scraping_ = false;
    bool ceilHold_ = false;
    int scrapes_ = 0;
    float hull_ = 100.f;
    float y_ = 110.f;
    float vy_ = 0.f;
    float speed_ = 0.f;
    float halfW_ = 20.f;
    float sonar_ = 0.f;
    float pingCd_ = 0.f;
    float blowCd_ = 0.f;
    float siltT_ = 0.f;
    float riseT_ = 0.f;
    float minClear_ = 1000.f;
    double x_ = 0;
    double t_ = 0;
    double run_ = 0;
    double chimeT_ = 0;
    int chime_ = -1;
    const char* note_ = "";
    std::vector<Bit> motes_;
};

}  // namespace sub
