// S3 SKATE GOLD
//   s3skategold                 land the line; only the gold counts double
//   s3skategold --sim           autopilot, exits 0 only on the double
//   s3skategold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/skategold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    skategold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool shotRun = false;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!shotRun && frames == 16) {
            save(sys, "title.png");
            shotRun = true;
        }
        if (shotDir && frames == 180) save(sys, "line.png");
    }
    save(sys, "end.png");
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.finisherGold() && math && cart.gold() >= 1 && cart.score() >= skategold::Game::kLine &&
        cart.bare() < skategold::Game::kLine && cart.line() == skategold::Game::kTricks) {
        std::printf(
            "S3 SKATE GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  line %d  (%.1f s)\n",
            cart.gold(), cart.cream(), cart.score(), cart.line(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SKATE GOLD  NO DOUBLE  %s  gold %d  cream %d  score %d  line %d/%d  falls %d  (%.1f s)\n",
                cart.fail(), cart.gold(), cart.cream(), cart.score(), cart.line(), skategold::Game::kTricks,
                cart.falls(), frames / 60.0);
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
            std::printf("S3 SKATE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skategold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skategold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
