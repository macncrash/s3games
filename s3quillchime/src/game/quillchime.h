// S3 QUILLCHIME — a short quill. The hour has to chime.
// The nib walks a desk toward a brass bell. Only a short stroke, laid while
// the clock stands on twelve, makes the hour chime. A long stroke snaps the
// quill. Three dips, then the hour is gone.
#pragma once

#include <vector>

#include "console/system.h"

namespace quillchime {

constexpr int kStartSec = 11 * 3600 + 59 * 60 + 45;
constexpr int kHourSec = 12 * 3600;
constexpr int kGraceSec = 4;
constexpr int kFpc = 20;
constexpr int kInkLo = 16;
constexpr int kInkHi = 36;
constexpr int kDips = 3;
constexpr float kRingX = 246.f;
constexpr float kRingY = 86.f;
constexpr float kRingR = 16.f;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUILLCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int ink() const { return ink_; }
    int dips() const { return dips_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* face() const { return face_; }

private:
    enum class Mode { Title, Write, Gap, Chime, Fail, Over };

    struct Dot {
        float x, y;
    };

    void buildArt();
    void toTitle();
    void begin();
    void commit();
    void snap();
    void beginChime();
    void beginGap(const char* why);
    void beginFail(const char* why);

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    bool inRing() const;
    void faceTime(int& h, int& m, int& s) const;

    void bot();
    void draw();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool nib = false);
    void words(const gs::Image& img, float cx, float cy);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    const char* face_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inking_ = false;
    int ink_ = 0;
    int stroke_ = 0;
    int dips_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int titleWait_ = 0;
    float x_ = 72.f;
    float y_ = 168.f;
    float prevX_ = 72.f;
    float prevY_ = 168.f;
    float orbit_ = 0.f;
    std::vector<Dot> dots_;

    gs::Image quill_{};
    gs::Image blot_{};
    gs::Image bell_{};
    gs::Image clapper_{};
    gs::Image pip_{};
    gs::Image well_{};
    gs::Image title_{};
    gs::Image hint_{};
    gs::Image start_{};
    gs::Image chime_{};
    gs::Image lost_{};
    gs::Image shortW_{};
    gs::Image digit_[10]{};
    gs::Image colon_{};
};

}  // namespace quillchime
