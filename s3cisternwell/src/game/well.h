// S3 CISTERNWELL — keep the stone well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace well {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERNWELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int integrity() const { return hp_; }
    // 0 title, 1 a wave, 2 between waves, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Dead, Victory };

    struct Foe {
        float x, vx, flash;
        int hp, kind;
    };
    struct Drop {
        float x, vx, life;
    };
    struct Spawn {
        float t;
        int dir, hp, kind;
        float spd;
    };

    void bootWatch();
    void armWave();
    void update(float dt);
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void spray();
    void botAim(bool& fire);
    void blip(bool high);
    void crack();
    void fanfare();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int score_ = 0;
    int hp_ = 8;
    int hpMax_ = 8;
    float px_ = 160;
    int face_ = 1;
    float cool_ = 0;
    float t_ = 0;
    float hold_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    int spawnAt_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Drop> drops_;
};

}  // namespace well
