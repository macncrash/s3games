// S3 HARBOR RELIEF — one harbor. Hold until the relief bell. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace harbor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int turned() const { return turned_; }
    int piers() const { return piers_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };

    struct Spawn {
        float t;
        int kind;
        float lat;
    };
    struct Boat {
        int kind = 0;
        float lat = 0;
        float z = 0;
        float prev = 0;
        float age = 0;
        int points = 0;
        bool on = true;
        bool turned = false;
    };
    struct Splash {
        float lat = 0, z = 0, t = 0;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void steer(float& axis, bool& hold, bool& answer);
    int soonest() const;
    void stopBoat(Boat& b);
    void enterHarbor();
    void winWatch();
    void loseWatch(const char* why);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void project(float lat, float z, float& x, float& y) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    bool holding_ = false;
    const char* reason_ = "";
    int score_ = 0;
    int turned_ = 0;
    int piers_ = 3;
    int spawnAt_ = 0;
    float lat_ = 0;
    float watch_ = 0;
    float modeT_ = 0;
    float bellTick_ = 0;
    float shake_ = 0;
    float answerLeft_ = 0;
    std::vector<Spawn> script_;
    std::vector<Boat> boats_;
    std::vector<Splash> splashes_;
};

}  // namespace harbor
