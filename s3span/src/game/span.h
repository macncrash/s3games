// S3 SPAN — hold the suspender bays until the column is on the far bank.
#pragma once
#include <array>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace span {

class Game : public gs::Cart {
public:
    static constexpr int kColumn = 7;

    const char* title() const override { return "S3 SPAN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int across() const { return across_; }
    int column() const { return kColumn; }
    float seconds() const { return clearT_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };
    enum class Kind { Banner, Pike, Cart };
    enum class Phase { Gap, Warn, Blow };

    struct Bay {
        float sag = 0;
        float load = 0;
    };
    struct Member {
        Kind kind = Kind::Pike;
        float x = 0;
        float load = 1;
        float drop = 0;
        bool fell = false;
        bool parked = false;
    };
    struct Mote {
        float x = 0, y = 0, a = 0;
    };

    void boot();
    void begin();
    void stepPlay(float dir, bool hold);
    void botIntent(float& dir, bool& hold, bool& go);
    void draw();
    void audio();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false);
    void word(const gs::Mipped& m, float cx, float cy, float h, int pal);
    int bayAt(float x) const;
    float bayCenter(int i) const;
    float footY(float x) const;
    float cableY(float u) const;
    float parkX(int i) const;
    int pickGust() const;
    int heaviest() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Gap;
    std::array<Bay, 5> bay_{};
    std::array<Member, kColumn> col_{};
    std::vector<Mote> motes_;
    bool bot_ = false;
    bool holding_ = false;
    bool over_ = false;
    bool won_ = false;
    bool splash_ = false;
    int across_ = 0;
    int gustBay_ = -1;
    int gustN_ = 0;
    int goal_ = 0;
    int fan_ = -1;
    float t_ = 0;
    float playT_ = 0;
    float clearT_ = 0;
    float phaseT_ = 0;
    float px_ = 90;
    float blip_ = 0;
    float blipF_ = 0;
    float shake_ = 0;
};

}  // namespace span
