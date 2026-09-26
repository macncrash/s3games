// S3 HOOPMARK — one court. The gold coin is the mark.
// Set it on the spot beyond the arc and shoot. A basket the rim counts opens
// the mark. Lifting the coin finishes it, and you leave. First to twenty-one
// is not the job. Four shots without a count leave the mark open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hoopmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOPMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool opened() const { return opened_; }
    bool left() const { return left_; }
    bool onMark() const { return onMark_; }
    bool flying() const { return mode_ == Mode::Flight; }
    int shots() const { return shots_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Place, Aim, Flight, Call, Lift, Leave, Pause, Over };
    enum class Call { Swish, Count, Bank, Rim, Short, Long, Air };

    struct Ball {
        float x = 0, y = 0, z = 0;
        float vx = 0, vy = 0, vz = 0;
    };
    struct Input {
        bool action = false, start = false, back = false;
        float x = 0, y = 0;
    };

    void resetCourt();
    void begin();
    void trySet();
    void launch(float meter);
    void stepBall(float dt);
    void fly(float dt);
    void finishShot();
    void afterCall();
    void tryLift();
    void finish();
    void fail();
    Call classify() const;
    static const char* nameOf(Call c);
    void aimHuman(float dt, const Input& in);
    void autoRelease(float dt);
    void dribble();
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void tone(int ch, float freq, float vol, float hold);
    void chord(float a, float b, float c, float hold);
    void pumpAudio();
    void say(const char* s, float time);
    int worth() const;
    float markDist() const;
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Place;
    Call call_ = Call::Air;
    Ball ball_{};
    const char* result_ = "none";
    const char* say_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onMark_ = false;
    bool opened_ = false;
    bool lifted_ = false;
    bool finished_ = false;
    bool left_ = false;
    bool scored_ = false;
    bool rimHit_ = false;
    bool bank_ = false;
    bool crossed_ = false;
    bool rimSnd_ = false;
    bool bankSnd_ = false;
    bool wasIn_ = false;
    bool faceLeft_ = false;
    int shots_ = 0;
    int shotPts_ = 3;
    float feetX_ = 0;
    float aimZ_ = 0;
    float coinX_ = 0;
    float coinZ_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float t_ = 0;
    float flight_ = 0;
    float scoreAt_ = 0;
    float callT_ = 0;
    float flash_ = 0;
    float crossPast_ = 0;
    float watch_ = 0;
    float sayT_ = 0;
    float prevDrib_ = 0;
    float toneUntil_[3] = {};
};

}  // namespace hoopmark
