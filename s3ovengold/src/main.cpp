// S3 OVEN GOLD
//   s3ovengold                 a short oven, only the gold counts double
//   s3ovengold --sim           autopilot, exits 0 only on that double
//   s3ovengold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/oven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    ovengold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool shot = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!shot && frames == 8) {
            save(sys, "bake.png");
            shot = true;
        }
    }
    save(sys, "end.png");
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    const int bare = cart.bare();
    if (cart.won() && cart.finisherGold() && math && cart.gold() >= 1 && cart.score() >= cart.line() &&
        bare < cart.line() && cart.loaves() >= 1) {
        std::printf(
            "S3 OVEN GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  loaves %d  (%.1f s)\n",
            cart.gold(), cart.cream(), cart.score(), cart.loaves(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 OVEN GOLD  NO DOUBLE  gold %d  cream %d  score %d  loaves %d  %s %s  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.score(), cart.loaves(), cart.phase(), cart.say(), frames / 60.0);
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
            std::printf("S3 OVEN GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ovengold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ovengold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
