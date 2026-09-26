// S3 DART GOLD — 501. Only a gold bed counts as a double.
// The cream double ring scores its face and cannot finish the leg.
// A leave of 1, 2, or 3 busts the visit: those have no gold double.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace dartgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DART GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool goldOut() const { return goldOut_; }
    int left() const { return remain_; }
    int darts() const { return thrown_; }
    int cream() const { return cream_; }
    const char* out() const { return out_; }
    const char* last() const { return last_; }
    const char* want() const { return want_; }

private:
    enum class Mode { Title, Aim, Flight, Show, Bust, Win, Pause };

    struct Bed {
        int score = 0;
        int face = 0;
        bool dbl = false;
        bool gold = false;
        int sector = 0;
        float rad = 0;
        int paint = 0;
        char name[8] = {};
    };
    struct Pin {
        float x = 0, y = 0;
        bool on = false;
    };

    void buildBeds();
    void label(const Zone& z, char* out) const;
    bool audit();
    bool proof() const;
    bool finish(int remain, int darts, Bed* plan, int& n) const;
    Bed bestSetup(int remain) const;
    void place(int sector, float rad, float& x, float& y) const;
    void place(const Bed& b, float& x, float& y) const;
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
    bool sweet() const;
    uint32_t rnd();

    void tickAudio(float dt);
    void draw();
    void backdrop();
    void blit(const gs::Image& img, float cx, float cy, int w, int h, int pal);
    void dartAt(float x, float y, float h, int slot);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

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
    bool goldOut_ = false;
    bool botMiss_ = false;
    bool busting_ = false;
    bool closeVisit_ = false;
    int remain_ = 501;
    int visitStart_ = 501;
    int thrown_ = 0;
    int visitDart_ = 0;
    int visitN_ = 0;
    int cream_ = 0;
    char visit_[3][8] = {};
    char last_[8] = {};
    char want_[8] = {};
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
    uint32_t rng_ = 0xD6A7u;
    Pin pin_[3];
};

}  // namespace dartgold
