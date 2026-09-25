// S3 RIDGE — they come over the crest. Hold the path.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace ridge {

enum class Kind { Raider, Brute, Runner };

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stakes() const { return stakes_; }
    int stopped() const { return stopped_; }
    int crest() const { return crest_; }
    // 0 title, 1 fighting, 2 between crests, 3 held, 4 lost
    int marker() const;

private:
    enum class Mode { Title, Brief, Fly, Pause, Clear, Victory, Lost };

    struct Spawn {
        float t, u;
        Kind kind;
    };
    struct Foe {
        Kind kind;
        float u, z, age, flash;
        int hp, points;
        bool on;
    };
    struct Bolt {
        float u, z;
        bool on;
    };
    struct Puff {
        float u, z, t;
    };
    struct Pop {
        float x, y, t;
        int pts;
    };
    struct Spot {
        float x, y, h;
        int fog;
    };

    void beginWatch();
    void fillSpawns();
    void applySky();
    void update(float dt);
    void draw();
    void loose();
    void stop(Foe& f, bool atLine);
    void breach();
    void fanfare(bool big);
    void tickAudio();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void shadow(float cx, float cy, float w);
    void puffAt(float u, float z);
    void popAt(float x, float y, int pts);
    float halfW(float row) const;
    float bend(float row) const;
    float rowAt(float z) const;
    float laneX(float u) const;
    float playerU() const;
    float holdNow() const;
    float speed() const;
    Spot spot(float u, float z, float base) const;
    const gs::Mipped& foeImg(Kind k, int frame) const;
    const char* crestName() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool plant_ = false;
    bool fanBig_ = false;
    int crest_ = 0;
    int score_ = 0;
    int stakes_ = 4;
    int stopped_ = 0;
    int cursor_ = 0;
    int spawned_ = 0;
    int fanStep_ = -1;
    float modeT_ = 0;
    float waveT_ = 0;
    float scroll_ = 0;
    float px_ = 160;
    float vx_ = 0;
    float fireCd_ = 0;
    float flashT_ = 0;
    float shake_ = 0;
    float shx_ = 0, shy_ = 0;
    float beep_ = 0, cue_ = 0, fanT_ = 0;
    uint16_t skyTop_ = 0, skyHor_ = 0, skyDrop_ = 0;
    std::vector<Spawn> spawns_;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
    std::vector<Puff> puffs_;
    std::vector<Pop> pops_;
};

}  // namespace ridge
