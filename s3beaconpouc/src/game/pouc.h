// S3 BEACON POUC — you have the beacon. Carry the pouch across.
#pragma once
#include "console/gfx.h"
#include "console/system.h"

namespace pouc {

enum Pal {
    PAL_HUD = 0,
    PAL_NIGHT = 1,
    PAL_WOOD = 2,
    PAL_POUCH = 3,
    PAL_HERO = 4,
    PAL_LAMP = 5,
    PAL_SEA = 6,
    PAL_IRON = 7,
    PAL_ALERT = 8,
    PAL_GO = 9
};

struct Art {
    gs::Mipped hero, pouch, lamp, flame, plank, barge, tower, post;
    gs::Mipped glyph[96];
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEACON POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the piers, 2 carrying, 3 the pouch crossed, 4 the watch is over
    int marker() const;
    const char* reason() const { return reason_; }
    void dump(const char* where) const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Barge {
        float mid = 0, amp = 0, w = 1.1f;
        float cx = 0, prev = 0;
    };

    void buildArt();
    void begin();
    void finish(bool crossed, const char* why);
    void bot(bool& left, bool& right, bool& jump, bool& shield);
    void stepPlay(bool left, bool right, bool jump, bool shield);
    bool pierAt(float x) const;
    int bargeAt(float x) const;
    const char* hint() const;
    void draw();
    void spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool feet);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Barge barge_[2]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    bool riding_ = false;
    int face_ = 1;
    int phase_ = 0;
    int plant_ = 0;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vy_ = 0;
    float pouchX_ = 0;
    float flame_ = 1.f;
    float cam_ = 0, t_ = 0, watch_ = 0;
    float rideDx_ = 0;
    int rideId_ = -1;
};

}  // namespace pouc
