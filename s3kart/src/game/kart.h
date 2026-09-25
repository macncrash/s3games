#pragma once

#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"
#include "game/track.h"

namespace kart {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KART"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int place() const { return place_; }
    int laps() const { return laps_; }

private:
    enum class Mode { Title, Count, Race, Pause, Win, Lose };
    enum class Kind { Tree, Stand, Flag };

    struct Racer {
        float s = 0, n = 0, speed = 0, steer = 0;
        float vmax = 46, line = 0.3f, bias = 0;
        int art = 0, pal = PAL_PLAYER;
        bool player = false;
        bool wall = false;
    };
    struct Prop {
        float s = 0, n = 0;
        Kind kind = Kind::Tree;
    };
    struct Bill {
        float z = 0, x = 0, y = 0, w = 0, h = 0;
        gs::Image img{};
        int pal = 0, fog = 0;
        bool shadow = false, flip = false;
    };

    void begin();
    void buildProps();
    void present(bool giant, bool real);
    void physics(float dt);
    void drive(Racer& k, float gas, float brake, float steer, float dt);
    void bump();
    float autoSteer(int ix) const;
    int placeNow() const;
    void finish(bool win);
    void chase(float s, float n);
    void sky();
    void road();
    void queueWorld(bool giant, bool real);
    void queueKart(int artIx, int pal, float s, float n, float lean, bool giant);
    void queueProps();
    void queueSky();
    void flush();
    void blit(const gs::Image& img, float x, float y, float w, float h, int pal, int fog, bool shadow, bool flip);
    void hudText(const std::string& s, float x, float y, int pal, int scale);
    void hudCenter(const std::string& s, float y, int pal, int scale);
    float textWidth(const std::string& s, int scale) const;
    void stampAt(const Stamp& s, float x, float y, int pal);
    void stampMid(const Stamp& s, float y, int pal);
    void hud();
    void audio();
    bool project(float wx, float wz, float& sx, float& sy, float& z) const;
    Racer& me() { return racers_[size_t(me_)]; }
    const Racer& me() const { return racers_[size_t(me_)]; }
    void readHuman(float& gas, float& brake, float& steer) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Racer> racers_;
    std::vector<Prop> props_;
    std::vector<Bill> bills_;
    int me_ = 0;
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Race;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool camReady_ = false;
    bool hit_ = false;
    int place_ = 6;
    int laps_ = 0;
    int count_ = 0;
    int banner_ = 0;
    int bannerN_ = 1;
    int blip_ = 0;
    int winAge_ = 0;
    float cine_ = 24.f;
    float gas_ = 0;
    float shake_ = 0;
    float shakeX_ = 0;
    float camX_ = 0, camZ_ = 0, camHdg_ = 0, focus_ = 0, horizon_ = HORIZON;
};

}  // namespace kart
