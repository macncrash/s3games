// S3 MINE — a cart in a tunnel. Shoot the props. Miss the walls.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace mine {

class Game : public gs::Cart {
public:
    static constexpr int kProps = 12;

    const char* title() const override { return "S3 MINE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int shot() const { return shot_; }
    int props() const { return kProps; }
    int wrecks() const { return wrecks_; }
    int lives() const { return lives_; }
    int score() const { return score_; }
    const char* endNote() const { return note_; }

private:
    enum class Mode { Title, Ride, Won, Lost };

    struct Prop {
        float z = 0, x = 0;
        int kind = 0;
        bool shot = false;
        bool flip = false;
    };
    struct Bolt {
        bool live = false;
        float z = 0, x = 0;
    };
    struct Spark {
        float x = 0, z = 0, h = 0, vx = 0, vz = 0, vh = 0, life = 0;
        int kind = 0;
    };

    void resetRun();
    void begin();
    void updateTitle();
    void updateRide();
    void botPlan(float& steer, bool& brake, bool& fire) const;
    bool hurt(const char* why);
    void win();
    void lose(const char* why);
    void spawnBolt();
    void moveBolt();
    void burst(const Prop& p);
    void audio();

    void draw();
    void skyRoad();
    void drawWorld();
    void drawCart();
    void hudText(int col, int row, const char* s, int pal);
    void hudCenter(int row, const char* s, int pal);
    void hudRight(int row, const char* s, int pal);
    void banner(const char* s, float cy, float destH, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float destH, int pal, int fog, bool hflip, bool foot,
             bool shadow);
    void sprBox(const gs::Mipped& m, float cx, float cy, float destW, float destH, int pal, int fog, bool shadow);
    void project(float x, float z, float h, float& sx, float& sy, float& dz) const;
    int fogOf(float dz) const;
    float daylight() const;
    void camera();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool brake_ = false;
    bool lock_ = false;
    int lives_ = 3;
    int wrecks_ = 0;
    int shot_ = 0;
    int score_ = 0;
    int stun_ = 0;
    int cool_ = 0;
    int flash_ = 0;
    int muzzle_ = 0;
    int warnT_ = 0;
    int tick_ = 0;
    float pz_ = 0, px_ = 0, vx_ = 0, shake_ = 0;
    float camZ_ = 0, camX_ = 0, shakeX_ = 0, shakeY_ = 0;
    const char* note_ = "";
    const char* warn_ = "";
    Prop props_[kProps]{};
    Bolt bolt_{};
    Spark sparks_[40]{};
};

}  // namespace mine
