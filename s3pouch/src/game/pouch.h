// S3 POUCH — carry it across three streets. Drop it and the run is over.
#pragma once
#include <string>

#include "console/system.h"
#include "pictures.h"
#include "world.h"

namespace pouch {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 POUCH"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool held() const { return held_; }
    int crossed() const { return cleared_; }
    float seconds() const { return over_ ? showSec_ : seconds_; }

private:
    enum class Mode { Title, Run, Pause, Dropped, Home };

    void begin();
    void updateRun();
    void botThink();
    void humanThink();
    void finish(bool win);
    void flyPouch();
    void draw(int frame);
    void banners(int frame);
    void cars(int frame);
    void props();
    void actor(int frame);
    void hud();
    void mix();
    void blip(float freq);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hudText(int col, int row, const std::string& s, int pal);
    float stepToward(float y, float dest) const;
    bool pathClear(float x, float y, float dest, int frame0) const;
    bool hits(float x, float y, int frame) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = true;
    bool commit_ = false;
    bool faceRight_ = true;
    int cleared_ = 0;
    int tick_ = 0;
    int wait_ = 0;
    int beep_ = 0;
    int fan_ = -1;
    int fanT_ = 0;
    int shake_ = 0;
    int endT_ = 0;
    float seconds_ = 0;
    float showSec_ = 0;
    float px_ = HOME_X;
    float py_ = POS_START;
    float dest_ = POS_START;
    float satX_ = 0, satY_ = 0, satVx_ = 0, satVy_ = 0;
};

}  // namespace pouch
