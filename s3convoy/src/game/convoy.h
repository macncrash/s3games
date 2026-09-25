// S3 CONVOY — the truck has to arrive. Lose it and the road ends.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace convoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CONVOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int score() const { return score_; }
    double seconds() const { return ticks_ / 60.0; }
    // 0 title, 1 on the road, 2 ended
    int phase() const;

private:
    enum class Mode { Title, Drive, Pause, Win, Dead };

    struct Obs {
        float z, x, hw;
        int kind;
        bool passed;
    };
    struct Raider {
        float x, z, side;
        int hp;
        bool live, woke;
    };
    struct Shot {
        float x, z;
    };
    struct Boom {
        float x, z, t;
    };
    struct Puff {
        float x, z, t;
    };
    struct Q {
        float key;
        gs::Sprite sp;
    };

    static constexpr int NOBS = 4;
    static constexpr int NRAID = 4;

    void resetCourse();
    void toTitle();
    void begin();
    void win();
    void lose();
    void titleFrame();
    void driveFrame();
    void pauseFrame();
    void endFrame();
    void human(float& steer, float& gas, bool& fire);
    void pilot(float& steer, float& gas, bool& fire);
    void physics(float steer, float gas, bool fire);
    void audio();
    void blip(bool high);
    void gun();
    void killRaider(int i);
    void hurt(int dmg, float x, float z, const char* why);
    void draw();
    void stamp(const gs::Mipped& m, float lat, float z, float worldW, int pal, bool flip, bool shadow, float bias);
    void screen(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip);
    void center(const gs::Mipped& m, float y, int pal);
    float text(const char* s, float x, float y, int pal, float scale);
    float textWidth(const char* s, float scale) const;
    void flush();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool engineOn_ = false;
    bool lostSnd_ = false;
    int menuWait_ = 0;
    int hull_ = 100;
    int score_ = 0;
    int ticks_ = 0;
    int frame_ = 0;
    int order_ = -1;
    int fanStep_ = -1;
    float truckX_ = -2.f, truckZ_ = 0.f;
    float jeepX_ = -2.f, jeepZ_ = -11.5f;
    float jeepV_ = 30.f;
    float attract_ = 0.f;
    float cliff_ = 1e9f;
    float shake_ = 0.f;
    float fireCd_ = 0.f;
    float blipT_ = 0.f;
    float fanT_ = 0.f;
    float camZ_ = 0.f, camLat_ = 0.f;
    bool blipHi_ = false;
    Obs obs_[NOBS]{};
    Raider raid_[NRAID]{};
    std::vector<Shot> shots_;
    std::vector<Boom> booms_;
    std::vector<Puff> puffs_;
    std::vector<Q> q_;
};

}  // namespace convoy
