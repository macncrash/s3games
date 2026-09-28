// S3 LOT WELL — at the lot, keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace lotwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOT WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int wave() const { return wave_; }
    int cracks() const { return cracks_; }
    int score() const { return score_; }
    // 0 title, 1 the watch, 2 the well stands, 3 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Banner, Play, Pause, Victory, Over };

    struct Ram {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool live = true;
    };
    struct Puff {
        float x = 0, y = 0, t = 0;
    };

    void begin();
    void openWave();
    void spawnOne();
    void driveHuman();
    void driveBot();
    void tryMove(float dx, float dy);
    void updatePlay();
    void shove(Ram& r);
    void crackWell(Ram& r);
    void clearWave();
    void win();
    void lose();
    void blip(float freq, float vol);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow = false, int fog = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float h, int pal);
    int waveCount() const;
    float waveSpeed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int cracks_ = 0;
    int score_ = 0;
    int spawned_ = 0;
    int cool_ = 0;
    int banner_ = 0;
    float t_ = 0;
    float px_ = 160.f;
    float py_ = 168.f;
    float face_ = 1.f;
    std::vector<Ram> rams_;
    std::vector<Puff> puffs_;
};

}  // namespace lotwell
