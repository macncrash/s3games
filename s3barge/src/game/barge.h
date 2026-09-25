// S3 BARGE — enter the lock, rise with the pound, leave without scraping.
#pragma once
#include "console/system.h"
#include "art.h"

namespace barge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BARGE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return playT_; }
    float risen() const { return risen_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float lateral() const { return lat_; }
    float speed() const { return vel_; }
    int phase() const { return int(phase_); }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };
    enum class Phase { Enter, Shut, Rise, Open, Leave };

    void begin();
    void step();
    void pilot(float& thrust, float& rudder) const;
    void controls(float& thrust, float& rudder);
    void wash(float& surge, float& sway) const;
    void hazards();
    bool hurt(const char* why);
    void audio();
    void draw();
    void drawWorld();
    void drawFender();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal);
    void mark(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void screen(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    float sx(float wx) const;
    float sy(float wh) const;
    float floatH() const;
    float keelH() const;
    float roofH() const;
    float bow() const;
    float stern() const;
    float loBottom() const;
    float upBottom() const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Enter;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float t_ = 0;
    float anim_ = 0;
    float playT_ = 0;
    float leaveT_ = 0;
    float settle_ = 0;
    float x_ = 0;
    float vel_ = 0;
    float lat_ = 0;
    float latV_ = 0;
    float chamber_ = 0;
    float risen_ = 0;
    float gateLo_ = 0;
    float gateUp_ = 1;
    float camX_ = 0;
    float thrust_ = 0;
    float rudder_ = 0;
    float blipT_ = 0;
    float chimeT_ = 0;
    float shake_ = 0;
};

}  // namespace barge
