// S3 QUARRY BANN — you have the quarry. Bring the banner back.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace qbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUARRY BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return has_; }
    // 0 title, 1 the haul, 2 banner in hand, 3 the banner is back, 4 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Prop {
        float z = 0, lat = 0, h = 1;
        int kind = 0;
    };
    struct Slide {
        float z = 0, amp = 2.f, w = 1.f, ph = 0;
    };
    struct Puff {
        float z = 0, lat = 0, age = 0, life = 0.4f;
    };
    struct Spot {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void buildProps();
    void update();
    void botDrive(float& steer, float& throttle);
    void win();
    void lose(const char* why);
    void blip(float freq);
    void serviceAudio();
    void draw();
    void layRoad(float shx);
    void drawProp(const Prop& pr, float shx);
    void drawCrew(float shx);
    void drawBanner(float z, float lat, float shx, bool carried);
    void drawSlide(int i, float shx);
    Spot project(float lat, float worldZ) const;
    float camZ() const;
    float bend(float worldZ) const;
    int fogFor(float worldZ) const;
    float slideLat(const Slide& s) const;
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
    bool has_ = false;
    const char* reason_ = "THE BANNER IS NOT BACK";
    int lives_ = 3;
    int face_ = 1;
    float pz_ = 16.f;
    float lat_ = 0;
    float vz_ = 0;
    float t_ = 0;
    float shift_ = 0;
    float plant_ = 0;
    float hold_ = 0;
    float inv_ = 0;
    float edge_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float puffT_ = 0;
    float commitZ_ = -1.f;
    float commitLat_ = 0;
    std::vector<Prop> props_;
    std::vector<Puff> puffs_;
    Slide slides_[3]{};
};

}  // namespace qbann
