// S3 GRASS — three left-hand circuits of a grass strip.
// Touch and go twice. The third landing is a full stop on the grass.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace grass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int circuit() const;
    const char* result() const { return result_[0] ? result_ : "IN THE AIR"; }
    float alt() const { return h_; }
    float speed() const { return v_; }
    float flightX() const { return x_; }
    float flightZ() const { return z_; }
    float flightHdg() const { return heading_; }
    bool pattern() const { return sawDownwind_; }
    bool departed() const { return sawDepart_; }
    int gate() const { return gate_; }

private:
    enum class Mode { Title, Fly, Pause, Dead, Over, Victory };
    enum class Roll { None, Go, Stop };
    enum class Kind { TreeA, TreeB, Post, Sock, Hangar, Cone, Bar };

    struct Prop {
        float x, z;
        Kind kind;
    };
    struct Puff {
        float x, z, t;
    };
    struct Bird {
        float x, z, y, ph;
    };
    struct Cloud {
        float x, z, y;
    };
    struct Sample {
        float t, x, z, h, v;
        int gate;
    };

    void buildWorld();
    void newGame();
    void place();
    void pilot(float& nose, float& steer, float& thr, bool& brake);
    void human(float& nose, float& steer, float& thr, bool& brake);
    void fly(float nose, float steer, float thr, bool brake);
    void note();
    void touchdown(float sink);
    void leaveGround();
    void groundChecks();
    void fail(const char* why);
    void succeed();
    void ambience();
    void draw();
    void drawRoad();
    void drawWorld();
    void drawCraft();
    void drawHud();
    const char* hint() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false,
             bool shadow = false);
    bool project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog) const;
    void blip(bool high);
    void sample();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Roll rollout_ = Roll::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onGround_ = true;
    bool braking_ = false;
    bool sawDownwind_ = false;
    bool sawDepart_ = false;
    bool engine_ = false;
    int score_ = 0;
    int lives_ = 3;
    int circuitsDone_ = 0;
    int gate_ = 0;
    int propFrame_ = 0;
    char result_[48] = {};
    char banner_[32] = {};
    float x_ = 0, z_ = 0, h_ = 0, v_ = 0, vh_ = 0;
    float heading_ = 0, pitch_ = 0, bank_ = 0;
    float throttle_ = 0;
    float maxH_ = 0;
    float t_ = 0, deadT_ = 0, bannerT_ = 0, beep_ = 0, fanT_ = 0, shake_ = 0;
    float puffT_ = 0, airT_ = 0, sampleT_ = 0, travel_ = 0;
    int fanStep_ = -1;
    int sampleN_ = 0;
    Sample samples_[12] = {};

    std::vector<Prop> props_;
    std::vector<Puff> puffs_;
    std::vector<Bird> birds_;
    std::vector<Cloud> clouds_;
};

}  // namespace grass
