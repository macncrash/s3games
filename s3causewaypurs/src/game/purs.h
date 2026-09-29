// S3 CAUSEWAY PURSUIT — you have the causeway. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <array>
#include <string>

namespace cwpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stalled() const { return stalled_; }
    int fleet() const { return kFleet; }
    int boiler() const { return boiler_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the causeway, 2 a machine just stalled, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Play, Victory, Fail };
    static constexpr int kFleet = 4;
    static constexpr int kHorizon = 78;

    struct Mach {
        int kind = 0;
        int home = 0;
        int lane = 0;
        int hp = 4;
        float z = 20;
        float boost = 0;
        float think = 0;
        float cool = 0;
        bool running = true;
    };

    void boot();
    void begin();
    void stepPlay(int laneDir, bool drive);
    void botIntent(int& laneDir, bool& drive, bool& go);
    void rivalsThink();
    void physics(bool drive);
    void contact();
    void stallRival(int i);
    void winRoad();
    void loseRoad(const char* why);
    void serviceAudio();
    void draw();
    void road();
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);
    int nearest() const;
    int alive() const;
    void place(float z, int lane, float& x, float& y, float& h) const;
    const gs::Mipped& body(int kind) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::array<Mach, kFleet> mach_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "NOT THE LAST";
    int stalled_ = 0;
    int boiler_ = 8;
    int lane_ = 0;
    int wantLane_ = 0;
    float t_ = 0;
    float playT_ = 0;
    float seizeT_ = 0;
    float cool_ = 0;
    float shake_ = 0;
    bool drive_ = false;
};

}  // namespace cwpurs
