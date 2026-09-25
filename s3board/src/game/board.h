// S3 BOARD — a night switchboard. Match the mark before the lamp dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace board {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOARD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int made() const { return made_; }
    int drops() const { return dropsAll_; }
    // 0 title, 1 patching, 2 shift clear, 3 victory, 4 pulled off
    int marker() const;

private:
    enum class Mode { Title, Menu, Help, Brief, Play, Pause, Clear, Fail, Victory };

    struct Call {
        bool on = false;
        bool clearing = false;
        int trunk = 0;
        int line = 0;
        float life = 0;
        float maxLife = 0;
        float flash = 0;
    };
    struct Pop {
        bool on = false;
        float x = 0, y = 0, t = 0;
        int pal = 0;
        char text[8] = {};
    };

    void newRun(bool practice);
    void startShift();
    void beginPlay();
    void play(float dt);
    void steer(float dt);
    void botAct();
    void seat();
    void tick(float dt);
    void maintain(float dt);
    void finishShift();
    void connectCall(Call& c);
    void dropCall(Call& c);
    bool spawnOne();
    Call* trunk(int i);
    int liveCount() const;

    void audio(float dt);
    void blip(bool high);
    void buzz();
    void click();
    void chime();
    void fanTick(float dt);
    void pop(float x, float y, const char* s, int pal);
    void agePops(float dt);
    int irnd(int n);

    void draw();
    void drawTop();
    void drawWorld();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);
    void sprBox(const gs::Mipped& m, float x, float y, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool practice_ = false;
    bool over_ = false;
    bool won_ = false;
    int menu_ = 0;
    int shift_ = 0;
    int bank_ = 0;
    int row_ = 0;
    int held_ = -1;
    int connected_ = 0;
    int made_ = 0;
    int drops_ = 0;
    int dropsAll_ = 0;
    int need_ = 6;
    int maxLive_ = 1;
    float life_ = 12;
    float spawnGap_ = 0.4f;
    float spawnWait_ = 0;
    float t_ = 0;
    float rep_ = 0;
    float beep_ = 0;
    float ringT_ = 0;
    float sparkT_ = 0;
    float sparkX_ = 0;
    float sparkY_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    uint32_t rng_ = 0xB0ADu;
    Call calls_[JACKS];
    Pop pops_[6];
};

}  // namespace board
