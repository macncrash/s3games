// S3 BEACON PURSUIT — you have the beacon. Be the last machine still running.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace purs {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_OK = 2,
    PAL_YOU = 3,
    PAL_RIVAL = 4,
    PAL_DEAD = 5,
    PAL_BEAM = 6,
    PAL_POST = 7,
    PAL_FX = 8
};

struct Art {
    gs::Mipped you, rival, dead, beam, lamp, post;
    gs::Mipped glyph[96];
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEACON PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the pursuit, 2 a rival machine stalls, 3 the night has ended
    int marker() const;

private:
    enum class Mode { Title, Run, Victory, Over };

    struct Rival {
        float z = 1.1f;
        float lane = 0.5f;
        float life = 1.f;
        bool live = false;
        bool done = false;
        int age = 0;
    };

    void buildArt();
    void beginRun();
    void update(float dt);
    void botSteer();
    void draw();
    void project(float lane, float z, float& x, float& y, float& scale) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void lose(const char* why);
    void winRun();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE BEACON FAILED";
    int score_ = 0;
    int stalled_ = 0;
    int flash_ = 0;
    int stallMark_ = 0;
    int spawned_ = 0;
    float t_ = 0;
    float px_ = 0.5f;
    float heat_ = 0;
    float grace_ = 0;
    float rail_ = 0;
    bool burn_ = false;
    Rival pack_[4]{};
};

}  // namespace purs
