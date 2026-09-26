// S3 STANDARD — take the flag off the road and bring it back.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace standard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STANDARD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    bool carrying() const { return has_; }
    float heroZ() const { return pz_; }
    float flagZ() const { return flagZ_; }
    int face() const { return facing_; }
    float speed() const { return speed_; }
    // 0 title, 1 the road, 2 the flag is aboard, 3 the ride home, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Lorry {
        float z = 0, home = 0, lat = 0, speed = 0;
        int kind = 0;
    };
    struct Prop {
        float z = 0, lat = 0;
        int kind = 0;
    };
    struct Puff {
        float z = 0, lat = 0, age = 0;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void buildWorld();
    void layout();
    void begin();
    void update();
    void botPlan(float& steer, bool& gas, bool& brake, bool& turn);
    void readControls(float& steer, bool& gas, bool& brake, bool& turn);
    void grab();
    void collide();
    void hit(float push);
    void win();
    void lose();
    void blip(bool high);
    void fanfare(bool good);
    void serviceAudio();
    void draw();
    void road(float shx);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog);
    Proj project(float lat, float wz) const;
    int fogFor(float ahead) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::vector<Lorry> lorries_;
    std::vector<Prop> props_;
    std::vector<Puff> puffs_;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    bool wantTurn_ = false;
    bool fanGood_ = false;
    int facing_ = 1;
    int lives_ = 4;
    int fanStep_ = -1;
    float pz_ = 0, plat_ = 0, speed_ = 0;
    float flagZ_ = 0, flagLat_ = 0;
    float t_ = 0, hint_ = 0, banner_ = 0, lock_ = 0, inv_ = 0, stun_ = 0, shake_ = 0, skid_ = 0;
    float wantT_ = 0, steerVis_ = 0, puffAcc_ = 0, blip_ = 0, fanT_ = 0;
    float grabZ_ = 0, grabLat_ = 0, grabT_ = 0;
};

}  // namespace standard
