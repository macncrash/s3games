// S3 TRENCH COLUMN — you have the trench. Stop the column on the road.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace trench {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH COLUMN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int through() const { return through_; }
    int column() const { return kColumn; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the road, 2 the column has stopped, 3 the watch failed
    int marker() const;

private:
    enum class Mode { Title, Play, Victory, Fail };
    static constexpr int kColumn = 4;
    static constexpr float kCableZ = 18.f;
    static constexpr float kLipZ = 7.f;
    static constexpr float kSpace = 6.5f;
    static constexpr float kCruise = 6.4f;

    struct Truck {
        int slot = 0;
        float z = 0;
        float speed = 0;
        bool held = false;
    };

    void bootTitle();
    void begin();
    void spawn(bool scenic);
    void update();
    void plant();
    void leave();
    void win();
    void lose(const char* why);
    void blip(float freq);
    void serviceAudio();
    float leadZ() const;
    void draw();
    void layRoad();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool planted_ = false;
    bool sag_ = false;
    const char* reason_ = "THE TRENCH IS QUIET";
    int stopped_ = 0;
    int through_ = 0;
    int fanStep_ = -1;
    float t_ = 0;
    float hold_ = 0;
    float settle_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Truck> trucks_;
};

}  // namespace trench
