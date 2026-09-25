// GIGaBOY
//   s3gigaboy                 play the cul-de-sac
//   s3gigaboy --sim           ride the first shift to twelve deliveries
//   s3gigaboy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "cartver.h"
#include "game/gig.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        gig::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gig::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool street = false, play = false;
    const int limit = 60 * 130;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!street && frames == 80) {
            save(sys, "street.png");
            street = true;
        }
        if (!play && cart.delivered() >= 1) {
            save(sys, "gameplay.png");
            play = true;
        }
    }
    if (!play) save(sys, "gameplay.png");
    int c = cart.cents();
    if (c < 0) c = 0;
    std::printf("S3 GIGABOY  %s  %d deliveries on Sleepy Cul-de-Sac  stars %.1f %s  $%d.%02d\n",
                cart.won() ? "PASS" : "FAIL", cart.delivered(), double(cart.stars()),
                cart.stars() >= 1.f ? "still up" : "down", c / 100, c % 100);
    if (!cart.won()) {
        std::fprintf(stderr, "missed %d  ammo %d  time %.1f  %s\n", cart.missed(), cart.ammo(), double(cart.seconds()),
                     cart.note());
    }
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("GIGaBOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gigaboy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gig::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
