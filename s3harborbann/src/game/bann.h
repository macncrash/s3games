// S3 HARBOR BANN — you have the harbor. Bring the banner back.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>

namespace hbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return carrying_; }
    int hull() const { return hull_; }
    float boatZ() const { return z_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the run out, 2 the banner is aboard, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Won, Lost };

    struct Cutter {
        float z = 0;
        float phase = 0;
        float w = 0.03f;
        float amp = 0.7f;
    };

    struct Proj {
        float x = 0, y = 0, s = 1;
        bool ok = false;
    };

    void begin();
    void tick();
    void bot(float& steer, float& throttle);
    void move(float steer, float throttle);
    void hazards();
    void hoist();
    void dock();
    void win();
    void lose(const char* why);
    float cutterLat(int i) const;
    int gateAhead(bool outbound) const;

    void draw();
    void skyRoad();
    void world();
    void lettering();
    Proj project(float d, float lat) const;
    float dForY(float y) const;
    float halfAt(float y) const;
    int fogFor(float d) const;
    void spr(const gs::Image& img, float cx, float cy, float dh, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    void spr(const gs::Mipped& m, float cx, float cy, float dh, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    void text(const std::string& s, float x, float y, int pal, float scale = 1.f);
    void textC(const std::string& s, float y, int pal, float scale = 1.f);
    void plate(float x, float y, float w, float h);

    gs::System* sys_ = nullptr;
    Art art_{};
    Cutter cut_[3]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carrying_ = false;
    const char* reason_ = "THE BANNER IS STILL OUT";
    int hull_ = 3;
    int inv_ = 0;
    int clock_ = 0;
    int fan_ = 0;
    float z_ = 28.f;
    float lat_ = 0.f;
    float vz_ = 0.f;
    float bannerZ_ = 560.f;
    float shake_ = 0;
    float ox_ = 0, oy_ = 0;
};

}  // namespace hbann
