// S3 LUGE LOCK — pass one ice lock. A scrape on a gate fails the run.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace lugelock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceTime_; }
    const char* why() const { return why_; }
    // 0 title, 1 approach, 2 in the chamber, 3 clear of the upper gate, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Result };
    enum class Door { Shut, Opening, Open, Closing };

    struct Draw {
        float z;
        gs::Sprite s;
    };
    struct Proj {
        bool ok;
        float x, y, hw, fog, z;
    };

    void begin();
    void pilot(float& steer, bool& tuck, bool& brake);
    void physics(float steer, bool tuck, bool brake);
    void crossGates();
    void scrape(const char* why);
    void finish();
    void audio();
    void draw();
    void sky();
    void road();
    void gateAt(float z, float open, const char* label, bool showSign);
    void rider();
    void hud();
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void queue(float z, const gs::Sprite& s);
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow = false);
    void stretch(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog, float z);
    void ui(const gs::Image& img, float x, float y);
    Proj project(float worldZ, float roadX) const;
    float camZ() const { return playerZ_ - 7.2f; }

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Draw> draws_;

    Mode mode_ = Mode::Title;
    Door lower_ = Door::Shut;
    Door upper_ = Door::Shut;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool tucking_ = false;
    bool braking_ = false;
    const char* why_ = "";
    int hor_ = 78;
    int shakeX_ = 0;
    int shakeY_ = 0;

    float modeTime_ = 0;
    float raceTime_ = 0;
    float playerZ_ = 0;
    float prevZ_ = 0;
    float playerX_ = 0;
    float latV_ = 0;
    float speed_ = 0;
    float steerSm_ = 0;
    float lowerOpen_ = 0;
    float upperOpen_ = 0;
    float hold_ = 0;
    float fill_ = 0;
    float shake_ = 0;
    float chime_ = 0;
};

}  // namespace lugelock
