#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace quarry {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUARRY WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int wellHp() const { return hp_; }

private:
    enum class Mode { Title, Watch, Banner, Fail, Win };
    enum class Kind { Rock, Cart, Boulder };

    struct Threat {
        Kind kind;
        float x, vx;
        int hp;
        int dmg;
        int swingHit;
        bool alive;
    };

    void beginWatch();
    void openWave();
    void updateWatch();
    void think(float& ax, bool& swing);
    void draw();
    void scene();
    void text(const std::string& s, float x, float y, float scale, int pal, int align);
    void spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip = false);
    void blip(float freq, float vol);
    void hush();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int score_ = 0;
    int hp_ = 100;
    int spawned_ = 0;
    int spawnWait_ = 0;
    int swingT_ = 0;
    int swingCd_ = 0;
    int swingId_ = 0;
    int facing_ = 1;
    int banner_ = 0;
    int tone_ = 0;
    float px_ = 160;
    float shake_ = 0;
    float lean_ = 0;
    std::vector<Threat> threats_;
};

}  // namespace quarry
