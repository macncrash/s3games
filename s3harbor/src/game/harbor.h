// S3 HARBOR — one channel, forts on both banks, a finite magazine.
#pragma once
#include <array>
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace harbor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int magazine() const { return mag_; }
    int hull() const { return hull_; }
    int silenced() const { return silenced_; }
    int batteries() const { return 6; }
    const char* cause() const { return cause_.c_str(); }

private:
    enum class Mode { Title, Run, Won, Lost };

    struct Fort {
        float z = 0;
        int side = 1;
        bool alive = true;
        bool doomed = false;
        bool salvo1 = false;
        bool salvo2 = false;
        int flash = 0;
    };
    struct Shot {
        int fort = -1;
        float lat0 = 0, lat1 = 0, dEnd = 0;
        int life = 0, life0 = 1;
        bool kill = false;
    };
    struct Hostile {
        float z0 = 0, lat0 = 0, lat1 = 0;
        int life = 0, life0 = 1;
        int side = 1;
    };
    struct Fx {
        float z = 0, lat = 0;
        int life = 0, life0 = 1, kind = 0;
    };
    struct Grave {
        float z = 0;
        int side = 1;
        int life = 0;
    };
    struct Proj {
        float x = 0, y = 0, s = 1;
        bool ok = false;
    };

    void resetWorld();
    void update();
    void botAct(bool& left, bool& right, int& fire);
    void move(bool left, bool right, bool up, bool down);
    void tickShots();
    void salvos();
    void tickIncoming();
    void trigger(int side);
    void rules();
    void tickFx();
    void mix();
    void killFort(int i);
    void hole();
    void win();
    void lose(const char* why);
    int target(int side) const;
    bool inbound() const;

    void draw();
    void skyRoad();
    void hud();
    void combatSprites();
    void worldSprites(bool attract);
    Proj project(float d, float lat) const;
    float dForY(float y) const;
    float halfAt(float y) const;
    int fogFor(float d) const;
    void spr(const gs::Image& img, float cx, float cy, float dh, int pal, bool flip = false, int fog = 0, bool shadow = false);
    void spr(const gs::Mipped& m, float cx, float cy, float dh, int pal, bool flip = false, int fog = 0, bool shadow = false);
    void text(const std::string& s, float x, float y, int pal, float scale = 1.f);
    void textC(const std::string& s, float y, int pal, float scale = 1.f);
    void plate(float x, float y, float w, float h);
    void burst(float z, float lat, int kind);
    void blip(float freq, float vol, int hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::array<Fort, 6> fort_{};
    std::vector<Shot> shots_;
    std::vector<Hostile> incoming_;
    std::vector<Fx> fx_;
    std::vector<Grave> graves_;
    std::string cause_;
    bool bot_ = false;
    bool paused_ = false;
    bool over_ = false;
    bool won_ = false;
    int mag_ = 8;
    int hull_ = 5;
    int silenced_ = 0;
    int cool_ = 0;
    int inv_ = 0;
    int note_ = 0;
    int fan_ = 0;
    int muzzle_ = 0;
    float boatZ_ = 0;
    float boatX_ = 0;
    float vx_ = 0;
    float speed_ = 0.5f;
    float shake_ = 0;
    float ox_ = 0, oy_ = 0;
};

}  // namespace harbor
