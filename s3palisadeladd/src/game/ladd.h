// S3 PALISADE LADD — at the palisade, reach the far ladder. Miss it and the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace palisadeladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PALISADE LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 the near walk, 2 the rolling log, 3 the far ladder, 4 ended
    int marker() const;
    const char* reason() const { return reason_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void win();
    void lose(const char* why);
    void paint();
    void bot(bool& left, bool& right, bool& up, bool& jump);
    void play(float dt, bool left, bool right, bool up, bool jump);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet);
    void text(const char* s, float x, float y, float scale, int pal);
    float logX() const;
    int platAt(float x, float y) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = false;
    bool onLadder_ = false;
    bool gapJumped_[4] = {};
    bool logHop_ = false;
    int onPlat_ = 0;
    int face_ = 1;
    int fan_ = -1;
    const char* reason_ = "";
    float t_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float coyote_ = 0;
    float foot_ = 0;
    float shake_ = 0;
    float camX_ = 0;
};

}  // namespace palisadeladd
