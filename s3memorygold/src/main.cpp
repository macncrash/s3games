// S3 MEMORY GOLD
//   s3memorygold                 play until only the gold counts double
//   s3memorygold --sim           autopilot leaves on a gold double
//   s3memorygold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/memorygold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        memorygold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 4; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    memorygold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.golds() >= 1) {
            save(sys, "play.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    bool cleared = cart.won() && cart.left() && cart.finisherGold() && cart.score() >= cart.line() &&
                   cart.bare() < cart.line() && cart.golds() > 0 && cart.cream() < cart.line();
    if (cleared) {
        std::printf(
            "S3 MEMORY GOLD  DOUBLE  only the gold counts double  score %d  bare %d  gold %d  cream %d  turns %d  (%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.turns(), frames / 60.0);
        return 0;
    }
    std::printf("S3 MEMORY GOLD  OPEN  no gold double  score %d  bare %d  gold %d  cream %d  turns %d  %s  (%.1f s)\n",
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.turns(), cart.why(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MEMORY GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3memorygold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<memorygold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
