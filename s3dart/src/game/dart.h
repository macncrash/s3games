// S3 DART — 501, double out. The board is the whole game.
#pragma once

#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace dart {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DART"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int left() const { return remain_; }
    int darts() const { return thrown_; }
    const char* out() const { return out_; }

private:
    enum class Mode { Title, Aim, Flight, Show, Bust, Win, Pause };

    // One bed the rules can name. sector < 0 is a bull.
    struct Bed {
        int score = 0;
        bool dbl = false;
        int sector = 0;
        float rad = 0;
        char name[8] = {};
    };
    struct Hit {
        int score = 0;
        bool dbl = false;
        char name[8] = {};
    };
    struct Pin {
        float x = 0, y = 0;
        bool on = false;
    };

    void buildBeds();
    void addBed(int score, bool dbl, int sector, float rad, char kind);
    bool audit();
    bool finish(int remain, int darts, Bed* plan, int& n) const;
    Bed bestSetup(int remain) const;
    void place(const Bed& b, float& x, float& y) const;
    Hit scoreAt(float x, float y) const;
    const Bed& target() const;

    void begin();
    void enterAim();
    void clearPins();
    void launch();
    void stick();
    void afterMark();
    void moveAim();
    void botAim();
    bool wantsThrow() const;
    float pulse() const;
    uint32_t rnd();

    void tickAudio(float dt);
    void draw();
    void backdrop();
    void blit(const gs::Image& img, float cx, float cy, int w, int h, int pal);
    void boardSprite();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Bed> beds_;
    Bed plan_[3];
    Bed setup_{};
    int planN_ = 0;
    bool have_ = false;
    bool rules_ = false;

    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool busting_ = false;
    bool closeVisit_ = false;
    int remain_ = 501;
    int visitStart_ = 501;
    int thrown_ = 0;
    int visitDart_ = 0;
    int visitN_ = 0;
    char visit_[3][8] = {};
    char last_[8] = {};
    char out_[8] = {};

    float aimX_ = kCx, aimY_ = kCy;
    float landX_ = kCx, landY_ = kCy;
    float fromX_ = kCx, fromY_ = 0;
    float destX_ = kCx, destY_ = kCy;
    int flightT_ = 0;
    float showT_ = 0;
    float t_ = 0;
    float toneT_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    uint32_t rng_ = 0x501u;
    Pin pin_[3];
};

}  // namespace dart
