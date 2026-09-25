// S3 MILITIA — three waves. The well has to stand.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace militia {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILITIA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int well() const { return well_; }
    int wave() const { return wave_; }
    // 0 title or brief, 1 holding, 2 wave clear, 3 the torch wave, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Brief, Fight, Clear, Pause, Victory, Over };

    struct Spawn {
        float t;
        int kind;
        int gate;
        float j;
    };
    struct Foe {
        int kind;
        float x, y, speed, rad, tall, age, flash;
        int hp, dmg, pts;
    };
    struct Shot {
        float x, y, vx, vy, life;
    };
    struct Puff {
        float x, y, life, h;
    };
    struct Popup {
        float x, y, life;
        int pts;
    };

    void boot();
    void prepareWave();
    void takePost();
    void update(float dt);
    void botAct(float dt);
    void humanAct(float dt);
    void steer(float tx, float ty, float dt);
    void fireShot();
    void swingButt();
    void damageFoe(int i, int dmg);
    void breach(int dmg);
    void win();
    void fanfare(bool big);
    void blip(bool high);
    void draw();
    void fadeFx();
    void serviceAudio();
    void applySky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet = false);
    void shadowAt(float x, float y, float w);
    const char* waveName() const;
    const char* waveOrder() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool moved_ = false;
    int wave_ = 0;
    int score_ = 0;
    int well_ = 10;
    int cursor_ = 0;
    float t_ = 0;
    float px_ = 160, py_ = 172;
    float faceX_ = 0, faceY_ = -1;
    float reload_ = 0, butt_ = 0, muzzle_ = 0, shake_ = 0, hurtFlash_ = 0;
    float blip_ = 0, shotTone_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanBig_ = false;
    uint16_t sky_ = 0;
    std::vector<Spawn> spawns_;
    std::vector<Foe> foes_;
    std::vector<Shot> shots_;
    std::vector<Puff> puffs_;
    std::vector<Popup> pops_;
};

}  // namespace militia
