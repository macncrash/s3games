// S3 FISH GOLD
//   s3fishgold                 a short fish, until only the gold counts double
//   s3fishgold --sim           autopilot leaves on a gold double
//   s3fishgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fishgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    fishgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, wet = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!titled && frames >= 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!wet && m == 1) {
            save(sys, "water.png");
            wet = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.goldOut() && cart.score() >= cart.line() && cart.bare() < cart.line() && cart.golds() > 0 &&
        cart.cream() == 0) {
        std::printf(
            "S3 FISH GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  fish %d  "
            "line %d  (%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.caught(), cart.line(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FISH GOLD  SHORT  score %d  bare %d  golds %d  cream %d  fish %d  line %d  (%.1f s)\n", cart.score(),
                cart.bare(), cart.golds(), cart.cream(), cart.caught(), cart.line(), frames / 60.0);
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
            std::printf("S3 FISH GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fishgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fishgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
