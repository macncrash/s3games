// S3 CAUSEWAY WELL — one stone causeway, one well at the near end.
// Three waves come down the road. Keep the well standing.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace cwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int well() const { return well_; }
    // 0 title, 1 the causeway, 2 a wave is down, 3 the well still stands
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Pause, Won, Lost };

    struct Spawn {
        int frame;
        int lane;
        int kind;
        float speed;
    };
    struct Foe {
        int kind = 0;
        int lane = 0;
        int hp = 1;
        int points = 100;
        int age = 0;
        float z = 1.2f;
        float speed = 0.006f;
        bool alive = true;
    };
    struct Shot {
        int lane = 0;
        float z = 0;
        int life = 0;
    };

    void begin();
    void loadWave();
    void input();
    void playTick();
    void act(int& lane, bool& fire);
    void shoot();
    void blip(int ch, float freq);
    void road();
    void draw();
    void place(int lane, float z, float& x, float& y, float& depth) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    bool anyAlive() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int score_ = 0;
    int wave_ = 0;
    int well_ = 6;
    int lane_ = 0;
    int waveTime_ = 0;
    int spawnIx_ = 0;
    int calm_ = 0;
    int banner_ = 0;
    int cool_ = 0;
    int flash_ = 0;
    int wonAge_ = 0;
    float sway_ = 0;
    Shot shots_[8] = {};
    std::vector<Spawn> spawns_;
    std::vector<Foe> foes_;
};

}  // namespace cwell
