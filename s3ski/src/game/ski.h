// S3 SKI — gates on a downhill. Miss three and the run is over.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace ski {

class Game : public gs::Cart {
public:
    static constexpr int kGates = 12;

    const char* title() const override { return "S3 SKI"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int clean() const { return clean_; }
    int misses() const { return misses_; }
    int gateCount() const { return kGates; }

private:
    enum class Mode { Title, Run, Pause, Dead, Win };

    struct Gate {
        float z = 0, x = 0;
        bool red = true;
        bool done = false;
        bool clean = false;
    };

    struct Proj {
        float x = 0, y = 0, dz = 0;
        bool ok = false;
    };

    void layCourse();
    void beginRun();
    void poseTitle();
    void physics();
    void audio();
    void draw();
    void backdrop();
    void skierAt();
    void world();
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false, int fog = 0);
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip, int fog, bool shadow);
    Proj project(float wx, float wz) const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void say(const char* s, int pal, float time);
    void blip(float freq, float vol, int frames);
    float steerInput() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Gate gates_[kGates]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fallRight_ = false;
    int clean_ = 0;
    int misses_ = 0;
    int hold_ = 0;
    int blip_ = 0;
    int fanT_ = 0;
    int fanStep_ = -1;
    int sayPal_ = PAL_HUD;
    int tick_ = 0;
    float t_ = 0;
    float runT_ = 0;
    float sayT_ = 0;
    float z_ = 0, x_ = 0, vx_ = 0;
    float camX_ = 0;
    float finishZ_ = 0;
    float shake_ = 0;
    std::string say_;
};

}  // namespace ski
