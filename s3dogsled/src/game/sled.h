// S3 DOGSLED — night mail. The team turns with you. The last gate is the checkpoint.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sled {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DOGSLED"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int gatesTaken() const;
    int gateCount() const { return 5; }
    int spills() const { return spills_; }
    float elapsed() const { return elapsed_; }
    // 0 title, 1 run, 2 mid trail, 3 lantern in sight, 4 finished
    int marker() const;
    const char* outcome() const;
    void formatElapsed(char* dst, int n) const;

private:
    enum class Mode { Title, Run, Pause, Victory, Fail };
    enum class Why { None, Lamp, Open, Post };

    struct Gate {
        float s, x, open;
        bool finish;
        const char* name;
        bool taken, missed;
    };
    struct Prop {
        float s, x;
        int kind;  // 0 spruce, 1 rock, 2 hare, 3 cabin
    };
    struct Flake {
        float x, y, sp, sz;
    };
    struct Puff {
        float x, y, vx, vy, t;
    };
    struct Pop {
        float y, t;
        int pal;
        std::string text;
    };
    struct Cmd {
        float steer = 0;
        bool mush = false;
        bool whoa = false;
        bool hike = false;
    };

    void setupGates();
    void buildScenery();
    void enterTitle();
    void resetRun();
    void loadBest();
    void saveBest();
    void updateTitle(const gs::Pad& pad);
    void updatePause(const gs::Pad& pad);
    void updateRun(const gs::Pad& pad);
    void updateResult(const gs::Pad& pad);
    void updateFlakes();
    void updatePops();
    Cmd command(const gs::Pad& pad) const;
    Cmd botCmd() const;
    void win();
    void fail(Why why, const char* name);
    void spill();
    void chime(float freq);
    void yip();
    void fanfareTick();
    void pop(const char* s, int pal);
    void lights();
    void audio();
    void draw();
    void cacheBend();
    float bendTo(float dist) const;
    float curvature(float s) const;
    bool iceAt(float s) const;
    bool nearGate(float s, float r) const;
    int nextGate() const;
    bool project(float wx, float wz, float& sx, float& sy, float& scale, int& fog) const;
    void drawRoad();
    void drawTeam();
    void drawWorld();
    void drawSkyBits();
    void drawHud();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Why why_ = Why::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool newBest_ = false;
    bool windOn_ = false;
    int spills_ = 0;
    int fan_ = -1;
    int puffI_ = 0;
    int lightR_ = -1, lightG_ = -1, lightB_ = -1;
    float t_ = 0;
    float elapsed_ = 0;
    float best_ = 0;
    float s_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float lead_ = 0;
    float speed_ = 0;
    float breath_ = 1;
    float hike_ = 0;
    float stun_ = 0;
    float shake_ = 0;
    float paw_ = 0;
    float puffAcc_ = 0;
    float blip_ = 0;
    float yipT_ = 0;
    float fanT_ = 0;
    float camX_ = 0, camY_ = 0;
    float bendCache_[80] = {};
    const char* missName_ = "";
    Gate gates_[5]{};
    std::vector<Prop> scenery_;
    Flake flakes_[12]{};
    Puff puffs_[16]{};
    std::vector<Pop> pops_;
};

}  // namespace sled
