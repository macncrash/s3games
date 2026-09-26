// S3 YARD COLUMN — you have the yard. Stop the column on the road.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace ycol {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD COLUMN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int through() const { return through_; }
    int column() const;
    const char* reason() const { return reason_; }
    // 0 title, 1 the road, 2 the column has stopped, 3 anything else
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Rig {
        bool column = false;
        int slot = 0;
        float z = 0, lat = 0, speed = 0, cruise = 0, haltZ = 0;
        float side = 1;
        float puff = 0;
        bool passed = false;
        bool stopped = false;
        bool orderly = false;
        bool spooked = false;
        bool turning = false;
        bool berthed = false;
    };
    struct Puff {
        float z = 0, lat = 0, age = 0, life = 0.45f;
    };
    struct Prop {
        float z = 0, lat = 0, h = 1;
        int kind = 0;
    };
    struct Spot {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void buildProps();
    void lay(bool scenic);
    void update();
    void stepMule(Rig& r, bool block);
    void stepTruck(Rig& r, bool block);
    void tickPuffs();
    void win();
    void lose(const char* why);
    void blip(float freq);
    void fanfare(bool good);
    void serviceAudio();
    bool muleClear() const;
    bool muleIn() const;
    bool blockOn() const;
    float leadZ() const;
    const char* hint() const;
    void draw();
    void layRoad(float shx);
    void drawGantry(float shx);
    void drawProp(const Prop& pr, float shx);
    void drawRig(const Rig& r, float shx);
    Spot project(float wx, float wz) const;
    int fogFor(float z) const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fanGood_ = true;
    bool wasBlock_ = false;
    const char* reason_ = "ANYTHING ELSE";
    int stopped_ = 0;
    int through_ = 0;
    int fanStep_ = -1;
    float t_ = 0;
    float hook_ = 0;
    float drop_ = 0;
    float hookVel_ = 0;
    float dropVel_ = 0;
    float settle_ = 0;
    float hold_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    std::vector<Rig> rigs_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace ycol
