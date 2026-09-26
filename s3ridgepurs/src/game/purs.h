// S3 RIDGE PURSUIT — you have the ridge. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace ridgepurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int stopped() const { return stoppedCount(); }
    const char* reason() const { return reason_; }
    // 0 title, 1 the pursuit, 2 a machine just stopped, 3 the ridge has ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Machine {
        int kind = 0;
        int lane = 0;
        int dir = 1;
        int hp = 1;
        int maxHp = 1;
        float lat = 0;
        float z = 10;
        float hold = 2;
        float wind = 0;
        float cool = 0;
        float flash = 0;
        float wreck = 0;
        float phase = 0;
        bool running = true;
    };
    struct Shot {
        float lat = 0;
        float z = 0;
        float prev = 0;
        int from = -1;
    };
    struct Puff {
        float lat = 0;
        float z = 0;
        float age = 0;
        float life = 0.4f;
        int kind = 0;
    };
    struct Prop {
        float lat = 0;
        float z = 0;
        int kind = 0;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void resetWorld();
    void update(float dt);
    void botPlan(float& goal, bool& fire);
    void launchPlayer();
    void launchBolt(int index);
    void hurtPlayer(int dmg);
    void stopMachine(Machine& m);
    void winRidge();
    void loseRidge(const char* why);
    void serviceAudio();
    void draw();
    void layRoad(float shx);
    Proj project(float lat, float z) const;
    float bendAt(float z) const;
    int fogFor(float z) const;
    bool laneHot(float lat, float reach) const;
    int stoppedCount() const;
    int rivalsRunning() const;
    int quarry() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "NOT THE LAST";
    const char* banner_ = "";
    int hull_ = 8;
    float t_ = 0;
    float watch_ = 0;
    float px_ = 0;
    float fireCd_ = 0;
    float hurtT_ = 0;
    float shake_ = 0;
    float scroll_ = 0;
    float travel_ = 0;
    float grace_ = 0;
    float arm_ = 0;
    float bannerT_ = 0;
    float blip_ = 0;
    float slip_ = 0;
    float pull_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Machine> machines_;
    std::vector<Shot> shots_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace ridgepurs
