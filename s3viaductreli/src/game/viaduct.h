// S3 VIADUCT — one span. Hold the deck until the relief bell.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace viaduct {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 VIADUCT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };
    enum Kind { CLIMB = 0, FUSE = 1 };

    struct Spawn {
        float t;
        int kind;
        int arch;
    };
    struct Threat {
        int kind = 0;
        int arch = 0;
        float life = 1;
        float snuff = 0;
        int age = 0;
        bool alive = true;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void sky();
    void winWatch();
    void loseWatch(const char* why);
    void clearThreat(Threat& t, int pts);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float archX(int i) const;
    int urgent() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    const char* reason_ = "THE SPAN WAS LOST";
    int score_ = 0;
    int spawnAt_ = 0;
    int flash_ = 0;
    int fanStep_ = -1;
    float px_ = 160;
    float watch_ = 0;
    float cool_ = 0;
    float shake_ = 0;
    float bellTick_ = 0;
    float modeT_ = 0;
    float fanT_ = 0;
    float face_ = 1;
    std::vector<Spawn> script_;
    std::vector<Threat> threats_;
};

}  // namespace viaduct
