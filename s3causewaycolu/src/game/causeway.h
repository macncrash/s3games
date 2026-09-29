// S3 CAUSEWAY COLUMN — one causeway. Stop the column on the road.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace cway {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY COLUMN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int through() const { return through_; }
    int column() const { return kColumn; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the causeway, 2 the column has stopped, 3 the column left the road
    int marker() const;

private:
    enum class Mode { Title, Play, Victory, Fail };
    static constexpr int kColumn = 4;
    static constexpr float kGap = 0.09f;
    static constexpr float kDeck0 = 0.20f;
    static constexpr float kDeck1 = 0.78f;

    struct Truck {
        float z = 0;
        float speed = 0.12f;
        bool stopped = false;
    };

    void begin();
    void update();
    void plant();
    void raise();
    bool gateHolds() const;
    bool onDeck(float z) const;
    void win();
    void lose(const char* why);
    void draw();
    void layRoad();
    void project(float z, float lat, float& x, float& y, float& h) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void text(const char* s, float x, float y, float scale, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool planted_ = false;
    const char* reason_ = "THE CAUSEWAY IS OPEN";
    int stopped_ = 0;
    int through_ = 0;
    float t_ = 0;
    float gateZ_ = 0.62f;
    float hold_ = 0;
    float flash_ = 0;
    std::vector<Truck> trucks_;
};

}  // namespace cway
