// S3 FOUNDRY WELL — at the foundry, keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace foundrywell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int courses() const { return rings_; }
    int breaches() const { return breaches_; }
    int wave() const { return wave_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 3 the well stands, 4 the well fell
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Pause, Victory, Over };

    struct Cue {
        float t = 0;
        int gate = 0;
        float skew = 0;
        int kind = 0;
        float speed = 28;
    };
    struct Beast {
        int id = 0;
        int kind = 0;
        int hp = 1;
        int tagged = -1;
        float x = 0, y = 0;
        float speed = 28;
        float chew = 0;
        float anim = 0;
        float flash = 0;
        bool live = true;
        bool chewing = false;
    };
    struct Floater {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };
    struct Ash {
        float x = 0, y = 0, t = 0;
    };

    void bootAudio();
    void beginRun();
    void toTitle();
    void openWave();
    void scriptWave(int wave);
    void dropBeast(const Cue& c);
    void tickPlay(float dt);
    void tickBanner(float dt);
    void bot(float dt);
    void human(float dt);
    void stepSmith(float vx, float vy, float dt);
    void hammer();
    void strike(Beast& b);
    void fell(Beast& b);
    void crackWell(Beast& b);
    void victory();
    void defeat();
    void ashAt(float x, float y);
    void coolFx(float dt);
    void drone();
    void blip(float freq, float vol);
    void draw();
    void sky();
    void layFloor();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip);
    void shade(float x, float y, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    float urgency(const Beast& b) const;
    int byId(int id) const;
    int closest() const;
    float depth(float y) const;
    const char* waveName(int w) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool walking_ = false;
    bool faceLeft_ = false;
    const char* reason_ = "UNFINISHED";
    int wave_ = 0;
    int nextWave_ = 0;
    int rings_ = 3;
    int breaches_ = 0;
    int score_ = 0;
    int cueAt_ = 0;
    int lock_ = -1;
    int swingNo_ = 1;
    int nextId_ = 1;
    float x_ = 160.f;
    float y_ = 178.f;
    float t_ = 0.f;
    float tWave_ = 0.f;
    float bannerT_ = 0.f;
    float swingCd_ = 0.f;
    float swingT_ = 0.f;
    float shake_ = 0.f;
    float droneHz_ = -1.f;
    float camX_ = 0.f;
    float camY_ = 0.f;
    bool floorLaid_ = false;
    std::vector<Cue> script_;
    std::vector<Beast> beasts_;
    std::vector<Floater> floaters_;
    std::vector<Ash> ash_;
};

}  // namespace foundrywell
