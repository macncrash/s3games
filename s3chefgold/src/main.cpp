// S3 CHEF GOLD
//   s3chefgold                 play chef until only the gold counts double
//   s3chefgold --sim           autopilot leaves when only the gold counts double
//   s3chefgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/chefgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        chefgold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    chefgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool pass = false, rush = false, win = false;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!pass && cart.golds() >= 1) {
            save(sys, "pass.png");
            pass = true;
        }
        if (!rush && cart.golds() >= 3) {
            save(sys, "rush.png");
            rush = true;
        }
        if (!win && cart.won()) {
            save(sys, "end.png");
            win = true;
        }
    }
    if (!win) save(sys, "end.png");
    if (cart.won() && cart.goldOut() && cart.score() >= cart.line() && cart.bare() < cart.line() && cart.golds() > 0 &&
        cart.cream() == 0) {
        std::printf(
            "S3 CHEF GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  line %d\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.line());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CHEF GOLD  SHORT  score %d  bare %d  golds %d  cream %d  line %d  %s\n", cart.score(), cart.bare(),
                cart.golds(), cart.cream(), cart.line(), cart.reason()[0] ? cart.reason() : "TIME");
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
            std::printf("S3 CHEF GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3chefgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<chefgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
