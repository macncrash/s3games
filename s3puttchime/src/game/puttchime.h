// S3 PUTTCHIME — a short putt. The hour has to chime.
// The ball has to drop on the hour. A cup while the clock is still short does not count.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace puttchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PUTTCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool holed() const { return holed_; }
    bool rolling() const { return mode_ == Mode::Roll && rollFrames_ > 28 && rollFrames_ < 62; }
    int putts() const { return strokes_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Aim, Roll, Early, Miss, Chime, Fail, Over, Pause };
    enum class Halt { Fly, Rest, Drop };

    struct Lie {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool rest = true;
    };

    static Halt stepLie(Lie& b, float* arrive = nullptr, int* lips = nullptr);
    void showTitle();
    void newGame();
    void beginAim();
    void putt(float ang, float spd);
    void beginChime();
    void beginEarly();
    void beginMiss();
    void beginFail(const char* why);
    bool solve(float& ang, float& spd, int& travel);
    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void faceTime(int& h, int& m, int& s) const;
    void blip(float freq, float vol = 0.07f);
    void strikeBell(int n);
    void silenceTicks();
    void tickClock();

    void draw();
    void lawn();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false,
             bool feet = false);
    void image(const gs::Image& img, float cx, float cy, float h, int pal);
    void clockAt(float cx, float cy, float size);
    void aimLine(float ang, float spd);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Lie ball_{};
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool holed_ = false;
    bool swinging_ = false;
    bool botReady_ = false;
    bool solved_ = false;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int rollFrames_ = 0;
    int earlyFrames_ = 0;
    int missFrames_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int strokes_ = 0;
    int lastSec_ = -1;
    int tickLeft_ = 0;
    int strikes_ = 0;
    int botTravel_ = 0;
    float aim_ = -1.4f;
    float meter_ = 0;
    float meterDir_ = 1;
    float botAng_ = -1.4f;
    float botSpd_ = 160;
    float bellAmp_ = 0.15f;
    float bellPh_ = 0;
    float cloud_ = 0;
};

}  // namespace puttchime
