// S3 SALLY COLUMN — at the sally, stop the column on the road.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sally {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY COLUMN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int through() const { return through_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the road, 2 the column has stopped, 3 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Wagon {
        float z = 0;
        float lat = 0;
        float lane = 0;
        float speed = 0;
        bool stopped = false;
        float puff = 0;
    };
    struct Puff {
        float z = 0, lat = 0, age = 0, life = 0.45f;
    };
    struct Spot {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void lay(bool parked);
    void update();
    void halt();
    void winWatch();
    void lose(const char* why);
    void blip(float freq);
    void serviceAudio();
    int lead() const;
    bool covers(float lat) const;
    float botDir() const;
    bool botHalt() const;
    Spot project(float wx, float wz) const;
    int fogFor(float z) const;
    void road(float shx);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fanGood_ = true;
    const char* reason_ = "THE WATCH IS OPEN";
    int stopped_ = 0;
    int through_ = 0;
    int fanStep_ = -1;
    float t_ = 0;
    float flag_ = 0;
    float settle_ = 0;
    float hold_ = 0;
    float horn_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float fanT_ = 0;
    std::vector<Wagon> wagons_;
    std::vector<Puff> puffs_;
};

}  // namespace sally
