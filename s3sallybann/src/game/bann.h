// S3 SALLY BANN — you have the sally. Bring the banner back.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sallybann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return has_; }
    int lives() const { return lives_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the field, 2 the banner is in hand, 3 the sally has ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Bolt {
        float x = 0;
        float y = 0;
        bool on = false;
    };

    void bootTitle();
    void begin();
    void update();
    void win();
    void lose(const char* why);
    bool threat(float dir) const;
    void blip(float freq);
    void serviceAudio();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::vector<Bolt> bolts_;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    const char* reason_ = "THE BANNER IS STILL OUT";
    int lives_ = 3;
    int face_ = 1;
    float px_ = 24.f;
    float py_ = 190.f;
    float vy_ = 0.f;
    float bannerX_ = 292.f;
    float clock_ = 36.f;
    float spawn_ = 0.55f;
    float inv_ = 0.f;
    float hold_ = 0.f;
    float t_ = 0.f;
    float step_ = 0.f;
    float beep_ = 0.f;
    float shake_ = 0.f;
};

}  // namespace sallybann
