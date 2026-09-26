// S3 SPAN RELIEF — hold the span until the relief bell, then haul it.
#pragma once
#include <array>
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace spanreli {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPAN RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const;
    int held() const { return held_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };
    enum class Phase { Gap, Warn, Blow };
    enum class Kind { Drum, Coat, Wagon };

    struct Bay {
        float sag = 0;
        float load = 0;
    };
    struct Member {
        Kind kind = Kind::Coat;
        float x = 0;
        float station = 0;
        float load = 1;
        float drop = 0;
        bool fell = false;
    };
    struct Mote {
        float x = 0, y = 0, a = 0;
    };

    void boot();
    void begin();
    void place(bool posed);
    void reset();
    void stepWatch(float dir, bool brace, bool haul);
    void botIntent(float& dir, bool& brace, bool& haul, bool& go);
    void readPad(float& dir, bool& brace, bool& haul, bool& go);
    void winWatch();
    void loseWatch(const char* why);
    void draw();
    void audio();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false);
    int bayAt(float x) const;
    float bayCenter(int i) const;
    float footY(float x) const;
    float cableY(float u) const;
    int heaviest() const;
    int pickGust() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Gap;
    std::array<Bay, 5> bay_{};
    std::array<Member, 6> col_{};
    std::vector<Mote> motes_;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    bool bracing_ = false;
    bool hauling_ = false;
    bool faceLeft_ = false;
    bool inZone_ = false;
    bool peal_ = false;
    const char* reason_ = "THE WATCH RAN OUT";
    int held_ = 0;
    int gustBay_ = -1;
    int gustN_ = 0;
    int fanStep_ = -1;
    float t_ = 0;
    float watch_ = 0;
    float rope_ = 0;
    float px_ = 158;
    float phaseT_ = 0;
    float shake_ = 0;
    float endT_ = 0;
    float bellTick_ = 0;
    float marchT_ = 0;
    float tick_ = 0;
    float blip_ = 0;
    float blipF_ = 440;
};

}  // namespace spanreli
