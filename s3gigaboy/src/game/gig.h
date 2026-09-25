// GIGaBOY — twelve porches on a sleepy cul-de-sac. The marker has to sit on the mat.
#pragma once

#include <string>
#include <vector>

#include "console/system.h"
#include "game/pics.h"
#include "game/view.h"

namespace gig {

class Game : public gs::Cart {
public:
    const char* title() const override { return "GIGaBOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int delivered() const { return delivered_; }
    int missed() const { return missed_; }
    int ammo() const { return ammo_; }
    int cents() const { return cents_; }
    float stars() const { return stars_; }
    float seconds() const { return time_; }
    const char* note() const { return note_; }

private:
    enum class Mode { Title, Upgrades, Board, Play, Pause, Win, Lose };

    struct House {
        float lat = 0, z = 0, porchLat = 0, porchZ = 0, mailLat = 0, mailZ = 0;
        int side = 1, pal = 0;
        bool deliver = false, used = false, missed = false;
    };
    struct Hole {
        float lat = 0, z = 0;
        bool hit = false;
    };
    struct Crate {
        float lat = 0, z = 0;
        bool taken = false;
    };
    struct Car {
        float lat = 0, z = 0;
        bool go = false, hit = false;
    };
    struct Tree {
        float lat = 0, z = 0;
    };
    struct Lamp {
        float lat = 0, z = 0;
    };
    struct Toss {
        float lat0 = 0, z0 = 0, lat1 = 0, z1 = 0, t = 0;
    };
    struct Pop {
        std::string text;
        float lat = 0, z = 0, t = 0;
        int pal = PAL_GOLD;
    };

    void startShift();
    void spawnRow();
    void cull();
    void update();
    void botSteer();
    bool linedUp() const;
    House* nearest();
    const House* nearest() const;
    int landAt(float lat, float z, bool take);
    void score(int which, float lat, float z);
    void hurt(float stars, int cost, const char* why, float lat, float z);
    void finish(bool win);
    void draw();
    void drawWorld();
    void drawTitle();
    void drawPlayHud();
    void hudText(int col, int row, const std::string& s, int pal);
    void lineText(const std::string& s, float x, float y, float h, int pal, int align);
    float lineWidth(const std::string& s, float h) const;
    void blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool feet = false, int fog = 0,
              bool shadow = false);
    void cam(float lat, float z, float& sx, float& sy) const;
    void blip(float freq);
    void pop(const std::string& text, float lat, float z, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int menu_ = 0;
    int ammo_ = AMMO_MAX;
    int delivered_ = 0;
    int missed_ = 0;
    int combo_ = 0;
    int cents_ = 0;
    int best_ = 0;
    int row_ = 0;
    float stars_ = 5.f;
    float titleStars_ = 5.f;
    float time_ = 0;
    float t_ = 0;
    float px_ = 0, pz_ = 0, vx_ = 0, spd_ = 0;
    float stamina_ = 1.f;
    float hop_ = 0, hopY_ = 0;
    float iframes_ = 0;
    float shake_ = 0, shakeX_ = 0, shakeY_ = 0;
    float beep_ = 0;
    float banner_ = 0;
    float nextZ_ = 0;
    const char* note_ = "";
    std::vector<House> houses_;
    std::vector<Hole> holes_;
    std::vector<Crate> crates_;
    std::vector<Car> cars_;
    std::vector<Tree> trees_;
    std::vector<Lamp> lamps_;
    std::vector<Toss> toss_;
    std::vector<Pop> pops_;
};

}  // namespace gig
