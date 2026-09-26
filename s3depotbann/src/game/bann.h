// S3 DEPOT BANN — you have the depot. Bring the banner back.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>

namespace dbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return carrying_; }
    int lives() const { return lives_; }
    float heroX() const { return px_; }
    float bannerX() const { return carrying_ ? px_ : bannerX_; }
    int track() const { return track_; }
    int phase() const { return int(phase_); }
    const char* reason() const { return reason_; }
    // 0 title, 1 the run out, 2 the banner is aboard at the far shed, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    // Autopilot corridors. The three cuts never share a lane.
    enum class Phase { Out, Wait, Fetch, Return, Fork, Home };

    struct Cut {
        float x = 0, vx = 0, minX = 0, maxX = 0, half = 50;
        int track = 0;
        int pal = 0;
    };
    struct Puff {
        float x = 0, y = 0, vx = 0, t = 0;
    };

    void tune();
    void layout();
    void paint();
    void begin();
    void tickPlay(const gs::Pad& pad);
    void bot(float& throttle, int& sw);
    void trySwitch(int dir);
    void moveCuts();
    void collide();
    void couple();
    void win();
    void lose(const char* why);
    bool safe(int tr, float x, float margin) const;
    void blip(float freq, float vol);
    void serviceAudio();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Cut cuts_[3]{};
    Puff puffs_[6]{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Out;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carrying_ = false;
    bool atSiding_ = true;
    bool raised_ = false;
    bool horn_ = false;
    int lives_ = 3;
    int track_ = 1;
    int face_ = 1;
    int clock_ = 0;
    int switchCd_ = 0;
    int inv_ = 0;
    int song_ = -1;
    int songKind_ = 0;
    const char* reason_ = "THE BANNER IS STILL OUT";
    float px_ = 220;
    float vx_ = 0;
    float py_ = 159;
    float bannerX_ = 1660;
    int bannerTrack_ = 2;
    float cam_ = 0;
    float shake_ = 0;
    float t_ = 0;
};

}  // namespace dbann
