// S3 TRAP — put the Hook on the boat, then on places that are not a boat.
// Pitch holds angle of attack. Power holds the glideslope.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"
#include "game/sites.h"

namespace trap {

enum class Grade { None, Under, Ok, Fair, Bolter, Waveoff, Ramp, Crash, Edge };

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRAP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    int passes() const { return passes_; }
    bool boatOk() const { return boatOk_; }
    int marker() const;

private:
    enum class Mode { Title, Menu, Help, Pick, Brief, Fly, Grade, Pause, Victory };

    void resetPass();
    void step(float dt);
    void draw();
    void banners();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void finish(Grade g, int wire);
    void say(const char* line);
    void loadProgress();
    void saveProgress();
    bool passGrade(Grade g) const;
    const Site& site() const { return siteDef(site_); }

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool tour_ = true;
    bool over_ = false;
    bool boatOk_ = false;
    bool waved_ = false;
    bool bolter_ = false;
    bool onDeck_ = false;
    int menu_ = 0;
    int site_ = 0;
    int pick_ = 0;
    int wire_ = 0;
    int passes_ = 0;
    int unlocked_ = 1;
    Grade grade_ = Grade::None;
    float t_ = 0;
    float flyT_ = 0;
    float log_ = 1;
    float x_ = 0, y_ = 0, z_ = 800;
    float vx_ = 0, vy_ = 0, vz_ = 70;
    float pitch_ = 0, roll_ = 0;
    float throttle_ = 0.7f;
    float brake_ = 0;
    float prevAoa_ = 0;
    float shake_ = 0;
    float lsoT_ = 0;
    std::string lso_;
    uint32_t rng_ = 1;
};

}  // namespace trap
