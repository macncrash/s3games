// S3 SPAN COLUMN — one span. Stop the column on the road.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace scol {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPAN COLUMN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int through() const { return through_; }
    int column() const;
    const char* reason() const { return reason_; }
    // 0 title, 1 the span, 2 the column has stopped, 3 anything else
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Rig {
        int slot = 0;
        float z = 0, speed = 0, cruise = 0, haltZ = 0;
        float puff = 0;
        bool stopped = false;
        bool orderly = false;
    };
    struct Puff {
        float z = 0, age = 0, life = 0.45f;
    };
    struct Prop {
        float z = 0, u = 0, h = 1;
        int kind = 0;
    };
    struct Spot {
        float x = 0, y = 0, h = 0;
        int fog = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void buildProps();
    void lay(bool scenic);
    void update();
    void decide();
    void stepRig(Rig& r, bool hold);
    void tickPuffs();
    void win();
    void lose(const char* why);
    void blip(float freq);
    void fanfare(bool good);
    void serviceAudio();
    float leadZ() const;
    bool onDeck() const;
    bool beamDown() const;
    float leaf(float z) const;
    float swingPx(float z) const;
    const char* hint() const;
    void draw();
    void layRoad();
    Spot spot(float u, float z, float base) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    int horizon() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rolling_ = false;
    bool committed_ = false;
    bool seated_ = false;
    bool chimed_ = false;
    bool fanGood_ = true;
    const char* reason_ = "THE SPAN IS OPEN";
    int stopped_ = 0;
    int through_ = 0;
    int fanStep_ = -1;
    int hor_ = 76;
    float t_ = 0;
    float span_ = 0;
    float beam_ = 0;
    float settle_ = 0;
    float hold_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float motor_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Rig> rigs_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace scol
