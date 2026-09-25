// S3 CLAY — twenty-five birds. The last five are the match.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace clay {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLAY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int broken() const { return broken_; }
    int matchHits() const { return matchBroken_; }
    int birdNo() const { return bird_ + 1; }
    // 0 title, 1 a round bird in the air, 2 a match bird in the air, 3 finished
    int phase() const;

private:
    enum class Mode { Title, Ready, Fly, Hold, Banner, Over, Pause };

    struct Shard {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
        int kind = 0;
    };

    void begin();
    void armReady();
    void launch();
    void advance();
    void finish();
    void fire();
    void miss();
    void steer(float dt);
    void shatter(float sx, float sy);
    void puff(float sx, float sy);
    void ticks(float dt);
    void fanfare();
    void sample(int index, float t, float& x, float& y, float& z, float& sx, float& sy) const;
    void project(float x, float y, float z, float& sx, float& sy) const;
    bool inPattern(float sx, float sy, float y, float z) const;
    bool flightDone() const;
    void draw();
    void backdrop(float shx);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blip(int ch, float freq, float hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Ready;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shell_ = true;
    bool hit_ = false;
    int bird_ = 0;
    int broken_ = 0;
    int matchBroken_ = 0;
    int card_[25] = {};
    int fanStep_ = -1;
    int shardN_ = 0;
    float beadX_ = 160, beadY_ = 102;
    float age_ = 0;
    float release_ = -1;
    float holdT_ = 0;
    float bannerT_ = 0;
    float overT_ = 0;
    float titleT_ = 0;
    float clock_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    Shard shards_[8];
};

}  // namespace clay
