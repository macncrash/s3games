// S3 WICKETMARK — one pitch. The gold coin is the bowler's mark.
// Set it on the scratch and run in. A legal ball on middle that hits the
// wicket opens the mark. Walk back and lift the coin. That finished mark
// ends the cartridge. The rest of an over is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wicketmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKETMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool opened() const { return opened_; }
    bool onMark() const { return onMark_; }
    bool bowling() const { return mode_ == Mode::Flight; }
    int balls() const { return balls_; }
    const char* callName() const;

private:
    enum class Mode { Title, Mark, Set, Run, Flight, Call, Lift, Pause, Over };
    enum class Call { None, Wicket, Full, Off, Leg, NoBall };

    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
        bool up = false;
        bool down = false;
        float x = 0;
        float y = 0;
        float stick = 0;
    };

    void place();
    void begin();
    void trySet();
    void release();
    void enterCall();
    void beginLift();
    void tryLift();
    void finish();
    void fail();
    void stepRun(const Input& in);
    void stepFlight();
    void stepCall();
    void stepLift(const Input& in);
    bool inWindow() const;
    void ballAt(float u, float& x, float& y) const;
    const char* banner() const;
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void say(const char* s, float time);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shade = false, bool feet = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Mark;
    Call call_ = Call::None;
    const char* say_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onMark_ = false;
    bool opened_ = false;
    bool lifted_ = false;
    bool finished_ = false;
    bool loosed_ = false;
    bool broke_ = false;
    bool ticked_ = false;
    bool nicked_ = false;
    int line_ = 1;
    int balls_ = 0;
    float coinX_ = 0, coinY_ = 0;
    float bowlerX_ = 0, bowlerY_ = 0;
    float handX0_ = 0, handY0_ = 0;
    float clock_ = 0, setT_ = 0, flight_ = 0, callT_ = 0, sayT_ = 0, beep_ = 0, lineCool_ = 0, brokeAt_ = 0;
};

}  // namespace wicketmark
