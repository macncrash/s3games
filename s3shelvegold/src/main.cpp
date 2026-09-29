// S3 SHELVE GOLD
//   s3shelvegold                 play until only the gold counts double
//   s3shelvegold --sim           autopilot leaves on a gold double
//   s3shelvegold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/shelvegold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        shelvegold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    shelvegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    bool play = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!play && cart.marker() == 1 && cart.golds() >= 1) {
            save(sys, "play.png");
            play = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.goldOut() && cart.score() >= cart.line() && cart.bare() < cart.line() && cart.golds() >= 1) {
        std::printf(
            "S3 SHELVE GOLD  DOUBLE  only the gold counts double  score %d  bare %d  gold %d  cream %d  (%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SHELVE GOLD  SHORT  score %d  bare %d  gold %d  cream %d  back %d  (%.1f s)\n", cart.score(),
                cart.bare(), cart.golds(), cart.cream(), cart.returned(), frames / 60.0);
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
            std::printf("S3 SHELVE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shelvegold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shelvegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
