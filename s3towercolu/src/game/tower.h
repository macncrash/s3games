// S3 TOWER COLUMN — you have the tower. Stop the column on the road.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace twc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWER COLUMN"; }
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
        int kind = 0;
        bool column = false;
        int slot = 0;
        float z = 0, lat = 0, speed = 0, cruise = 0, haltZ = 0;
        float side = 1;
        float puff = 0;
        bool passed = false;
        bool stopped = false;
        bool orderly = false;
        bool spooked = false;
    };
    struct Puff {
        float z = 0, lat = 0, age = 0, life = 0.5f;
    };
    struct Prop {
        float z = 0, lat = 0, h = 1;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void lay(bool scenic);
    void update();
    void stepRig(Rig& r);
    void resolveChain();
    void win();
    void lose(const char* why);
    void blip(float freq);
    void fanfare(bool good);
    void serviceAudio();
    bool decoysClear() const;
    float leadZ() const;
    void draw();
    void road(float shx);
    void drawChain(float shx);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog);
    Proj project(float wx, float wz) const;
    int fogFor(float z) const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shut_ = false;
    bool dropping_ = false;
    bool judged_ = false;
    const char* reason_ = "THE COLUMN IS MOVING";
    int stopped_ = 0;
    int through_ = 0;
    float t_ = 0;
    float aim_ = 0;
    float chain_ = 1;
    float settle_ = 0;
    float hold_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Rig> rigs_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace twc
