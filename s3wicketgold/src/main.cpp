// S3 WICKET GOLD
//   s3wicketgold                 a short wicket; only the gold counts double
//   s3wicketgold --sim           autopilot, exits 0 only on that double
//   s3wicketgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wicketgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        wicketgold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    wicketgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool filed = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!filed && frames == 70) {
            save(sys, "over.png");
            filed = true;
        }
    }
    save(sys, "end.png");
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.finisherGold() && math && cart.gold() >= 1 && cart.score() >= cart.line() &&
        cart.bare() < cart.line() && cart.balls() >= 1) {
        std::printf("S3 WICKET GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  balls %d  (%.1f s)\n",
                    cart.gold(), cart.cream(), cart.score(), cart.balls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 WICKET GOLD  SHORT  no double  gold %d  cream %d  score %d  balls %d  %s  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.score(), cart.balls(), cart.say(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 WICKET GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wicketgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wicketgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
