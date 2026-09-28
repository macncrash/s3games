// S3 SHUFFLE TAPE — a short shuffle. The drawer has to match the tape.
#pragma once
#include <string>

#include "console/system.h"

namespace shuffletape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLE TAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* tapeLabel(int i) const;
    const char* drawerLabel(int i) const;

private:
    enum class Mode { Title, Aim, Slide, Pause, Win };

    struct Disk {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool live = false;
    };

    void buildArt();
    void tone(float freq, float vol);
    int zoneAt(float y) const;
    int settle(Disk d) const;
    void stepDisk();
    void commit(int zone);
    void draw();
    void blitText(const std::string& s, float x, float y, int pal);

    gs::System* sys_ = nullptr;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    Mode mode_ = Mode::Title;
    int filled_ = 0;
    int drawer_[3] = {};
    int pause_ = 0;
    int beep_ = 0;
    float aim_ = 160;
    float power_ = 0.55f;
    Disk disk_{};

    gs::Image board_{};
    gs::Image diskImg_{};
    gs::Image slip_{};
    gs::Image glyphs_[96]{};
};

}  // namespace shuffletape
