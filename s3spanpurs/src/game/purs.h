// S3 SPAN PURSUIT — you have the span. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <array>
#include <string>
#include <vector>

namespace spanpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPAN PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stalled() const { return stalled_; }
    int fleet() const { return kFleet; }
    int boiler() const { return mach_[0].running ? mach_[0].hp : 0; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the pursuit, 2 a machine just seized, 3 the span has ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    static constexpr int kN = 5;
    static constexpr int kFleet = 4;
    static constexpr int kBays = 5;

    struct Mach {
        int kind = 0;
        int hp = 1;
        int maxHp = 1;
        int face = 1;
        float x = 0;
        float home = 0;
        float speed = 40;
        float weight = 1;
        float vx = 0;
        float stun = 0;
        float cool = 0;
        float flash = 0;
        float press = 0;
        float drop = 0;
        bool running = true;
        bool fell = false;
        bool splashed = false;
    };
    struct Bay {
        float sag = 0;
        float load = 0;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, a = 0;
    };

    void boot();
    void begin();
    void step(float dir, bool ram);
    void botIntent(float& dir, bool& ram, bool& go);
    void readPad(float& dir, bool& ram, bool& go);
    void driveRivals();
    void doRam();
    void rivalRams();
    void separate();
    void edges();
    void springs();
    void stall(int i, bool fall);
    void hurtPlayer(float push);
    void winSpan();
    void loseSpan(const char* why);
    void puff(float x, float y, int n);
    void tickMotes();
    void steam();
    void serviceAudio();
    void draw();
    int quarry() const;
    int aliveRivals() const;
    int bayAt(float x) const;
    float bayCenter(int i) const;
    float sagAt(float x) const;
    float footY(float x) const;
    float cableY(float u) const;
    const gs::Mipped& body(int kind, int fr) const;
    int palFor(const Mach& m) const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::array<Mach, kN> mach_{};
    std::array<Bay, kBays> bay_{};
    std::vector<Mote> motes_;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "NOT THE LAST";
    std::string banner_;
    int stalled_ = 0;
    int score_ = 0;
    int focus_ = -1;
    int face_ = -1;
    float t_ = 0;
    float playT_ = 0;
    float px_ = 170;
    float cool_ = 0;
    float grace_ = 0;
    float shake_ = 0;
    float bannerT_ = 0;
    float blip_ = 0;
    float blipF_ = 440;
    float run_ = 0;
};

}  // namespace spanpurs
