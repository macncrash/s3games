// S3 TRENCH PURS — the last machine still running on the trench watch.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace purs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH PURS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int kills() const { return kills_; }
    int breaches() const { return breaches_; }

private:
    enum class Mode { Title, Watch, Win, Lose };

    struct Foe {
        float x, z, flash;
        bool alive;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void text(const std::string& s, float x, float y, int pal);
    void spr(const gs::Mipped& m, float cx, float footY, float h, int pal, int fog = 0, bool flip = false);
    float rnd();
    void botThink(bool& fire, bool& clear);
    void fail(const char* why);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int kills_ = 0;
    int breaches_ = 0;
    float aim_ = 160;
    float heat_ = 0;
    float run_ = 1;
    float idle_ = 0;
    float watch_ = 0;
    float spawn_ = 0;
    float shotCd_ = 0;
    float muzzle_ = 0;
    float t_ = 0;
    bool jammed_ = false;
    const char* why_ = "";
    uint32_t rng_ = 0x7A11u;
    std::vector<Foe> foes_;
};

}  // namespace purs
