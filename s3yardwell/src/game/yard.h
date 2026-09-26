// S3 YARD WELL — you have the yard. Keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace yardwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int courses() const { return courses_; }
    int breaches() const { return breaches_; }
    int wave() const { return wave_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 3 the well stands, 4 the well fell
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Pause, Victory, Over };

    struct Spawn {
        float t = 0;
        int side = 0;
        float along = 0;
        int kind = 0;
        float speed = 30;
    };
    struct Pest {
        int id = 0;
        int kind = 0;
        int hp = 1;
        int hitSerial = -1;
        float x = 0, y = 0;
        float speed = 30;
        float bash = 0;
        float anim = 0;
        float flash = 0;
        bool alive = true;
        bool bashing = false;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };
    struct Puff {
        float x = 0, y = 0, t = 0;
    };

    void bootAudio();
    void beginRun();
    void toTitle();
    void startWave();
    void buildScript(int wave);
    void spawnOne(const Spawn& s);
    void updatePlay(float dt);
    void updateBanner(float dt);
    void botAct(float dt);
    void human(float dt);
    void nudge(float vx, float vy, float dt);
    void swing();
    void hurt(Pest& p);
    void kill(Pest& p);
    void breach(Pest& p);
    void win();
    void lose();
    void puffAt(float x, float y);
    void fadeFx(float dt);
    void humForMode();
    void blip(float freq, float vol);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip);
    void shadowAt(float x, float y, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    float eta(const Pest& p) const;
    int findId(int id) const;
    int nearest() const;
    float zoom(float y) const;
    const char* waveName(int w) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool moving_ = false;
    bool faceLeft_ = false;
    const char* reason_ = "UNFINISHED";
    int wave_ = 0;
    int nextWave_ = 0;
    int courses_ = 3;
    int breaches_ = 0;
    int score_ = 0;
    int spawnAt_ = 0;
    int focus_ = -1;
    int swingSerial_ = 1;
    int fanStep_ = -1;
    int nextId_ = 1;
    float x_ = 160.f;
    float y_ = 176.f;
    float t_ = 0.f;
    float tWave_ = 0.f;
    float bannerT_ = 0.f;
    float swingCd_ = 0.f;
    float swingT_ = 0.f;
    float shake_ = 0.f;
    float endT_ = 0.f;
    float fanT_ = 0.f;
    float humFreq_ = -1.f;
    float camX_ = 0.f;
    float camY_ = 0.f;
    std::vector<Spawn> script_;
    std::vector<Pest> pests_;
    std::vector<Pop> pops_;
    std::vector<Puff> puffs_;
};

}  // namespace yardwell
