// S3 CHEFCHIME — play chef until the hour has to chime, then leave.
// The wall clock opens short of noon. Tend the roast. Plate and
// walk out only while the hour is chiming. Leaving earlier is a loss.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace chefchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 16;
constexpr int kTarget = 12;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 40;
constexpr int kPlates = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHEFCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    int hour() const;
    int minute() const;
    int second() const;
    int plates() const { return plates_; }
    float heat() const { return heat_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Early, Chime, Leave, Fail };

    void begin();
    void plate();
    void fail(const char* why);
    void botAct();
    void human(const gs::Pad& pad);
    bool audit() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    bool inGold() const { return heat_ >= kGoldLo && heat_ < kGoldHi; }
    void wallFace(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, float h, int pal, bool flip = false);
    void solid(float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    static constexpr float kGoldLo = 0.50f;
    static constexpr float kGoldHi = 0.88f;
    static constexpr float kRise = 0.0075f;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool pause_ = false;
    bool faceR_ = true;
    int plates_ = kPlates;
    int playFrames_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int swing_ = 0;
    float heat_ = 0.35f;
    float chefX_ = 96.f;
    float anim_ = 0.f;
};

}  // namespace chefchime
