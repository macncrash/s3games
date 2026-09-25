// S3 BATTERY — the road below. Stop the column before the gate.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace battery {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BATTERY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int through() const { return through_; }
    int column() const;
    // 0 title, 1 the road, 2 the column stops, 3 the column passes
    int marker() const;

private:
    enum class Mode { Title, Fight, Pause, Victory, Fail };

    struct Veh {
        int kind = 0;
        int hp = 1;
        float z = 0, lat = 0, speed = 0, flash = 0, smoke = 0;
        bool dead = false;
    };
    struct Shell {
        float x = 0, z = 0, t = 0, flight = 0;
        int target = -1;
        int gun = 0;
    };
    struct Boom {
        float x = 0, z = 0, age = 0, life = 0.5f;
    };
    struct Puff {
        float x = 0, z = 0, age = 0, hop = 0, life = 0.7f;
    };
    struct Prop {
        float z = 0, lat = 0;
        int kind = 0;  // 0 broadleaf, 1 pine, 2 gate post
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void beginFight();
    void buildProps();
    void spawnColumn();
    void update();
    void tickFx();
    void botAct();
    void humanAct();
    void shoot(int target);
    void explode(const Shell& s);
    void hit(Veh& v);
    void win();
    void lose();
    void fanfare(bool good);
    void blip(bool high);
    void serviceAudio();
    void draw();
    void road(float shx);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow);
    Proj project(float wx, float wz) const;
    float flightAt(float z) const;
    float predictZ(float z, float speed) const;
    int fogFor(float z) const;
    int incoming(int index) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int stopped_ = 0;
    int through_ = 0;
    int shots_ = 0;
    float t_ = 0;
    float hold_ = 0;
    float aimZ_ = 40;
    float aimLat_ = 0;
    float reload_ = 0;
    float shake_ = 0;
    float failFlash_ = 0;
    float blip_ = 0;
    float shotTone_ = 0;
    float gunKick_[2] = {};
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Veh> veh_;
    std::vector<Shell> shells_;
    std::vector<Boom> booms_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace battery
