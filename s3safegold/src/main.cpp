// S3 SAFE GOLD
//   s3safegold                 play
//   s3safegold --sim           set the dials; only the gold counts double
//   s3safegold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/safegold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System titleSys(true);
        safegold::Game title;
        titleSys.bootCart(title);
        for (int i = 0; i < 5; i++) titleSys.step();
        save(titleSys, "title.png");
    }

    gs::System sys(true);
    safegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    for (int i = 0; i < 8; i++) sys.step();
    save(sys, "room.png");

    int frames = 8;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    save(sys, "open.png");

    const bool math = cart.score() == cart.goldFace() * 2 + cart.creamFace();
    const bool creamMiss = cart.goldFace() + cart.creamFace() * 2 < cart.line();
    const bool dials = cart.solved();
    if (cart.won() && dials && cart.finisherGold() && math && creamMiss && cart.gold() >= 1 && cart.cream() >= 1 &&
        cart.score() == cart.line() && cart.bare() < cart.line() && cart.goldAt(2)) {
        std::printf(
            "S3 SAFE GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  line %d  %d-%d-%d  (%.1f s)\n",
            cart.gold(), cart.cream(), cart.score(), cart.line(), cart.dial(0), cart.dial(1), cart.dial(2),
            frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SAFE GOLD  STUCK  gold %d  cream %d  score %d  line %d  (%.1f s)\n", cart.gold(), cart.cream(),
                cart.score(), cart.line(), frames / 60.0);
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
            std::printf("S3 SAFE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3safegold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<safegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
