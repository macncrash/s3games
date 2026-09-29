// S3 REDOUBT — one redoubt. Stop the column on the road.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace redoubt {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 REDOUBT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int halted() const { return halted_; }
    int through() const { return through_; }
    int column() const;
    const char* reason() const { return reason_; }
    // 0 title, 1 the road, 2 the column has stopped, 3 the redoubt was overrun
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Truck {
        float z = 0, lat = 0;
        bool lead = false;
        bool stopped = false;
    };
    struct Puff {
        float z = 0, lat = 0, age = 0;
    };

    void bootTitle();
    void begin();
    void lay();
    void update();
    void botAim();
    void tryFire();
    void win();
    void lose(const char* why);
    void blip(float freq, float vol);
    void draw();
    void layRoad();
    bool project(float wx, float wz, float& sx, float& sy, float& ppm) const;
    int fogFor(float z) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip, int fog);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int halted_ = 0;
    int through_ = 0;
    const char* reason_ = "";
    float time_ = 0;
    float aimZ_ = 24;
    float aimX_ = 0;
    float cool_ = 0;
    float beep_ = 0;
    float shake_ = 0;
    std::vector<Truck> trucks_;
    std::vector<Puff> puffs_;
};

}  // namespace redoubt
