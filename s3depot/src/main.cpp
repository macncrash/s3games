// S3 DEPOT
//   s3depot                 clear the yard
//   s3depot --sim           autopilot stows every load before the whistle
//   s3depot --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/depot.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        depot::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depot::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 180;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.cleared() >= 2) {
            save(sys, "yard.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    int sec = cart.shiftSeconds();
    if (cart.won() && cart.cleared() >= cart.goal()) {
        std::printf("S3 DEPOT  PASS  yard clear  loads %d/%d  shift %d:%02d left  (%.1f s)\n", cart.cleared(),
                    cart.goal(), sec / 60, sec % 60, frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DEPOT  FAIL  %s  loads %d/%d  shift %d:%02d left  (%.1f s)\n", cart.why(), cart.cleared(),
                cart.goal(), sec / 60, sec % 60, frames / 60.0);
    std::fprintf(stderr, "fail tug %.1f %.1f\n", cart.tugX(), cart.tugY());
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
            std::printf("S3 DEPOT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depot [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depot::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
