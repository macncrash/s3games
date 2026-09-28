// S3 LOT PURSE
//   s3lotpurs                 play
//   s3lotpurs --sim           autopilot is the last machine still running
//   s3lotpurs --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/lot.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        lotp::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    lotp::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawLot = false, sawOne = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawLot && frames > 80) {
            save(sys, "lot.png");
            sawLot = true;
        } else if (m == 2 && !sawOne) {
            save(sys, "one.png");
            sawOne = true;
        } else if (m == 3 && !sawEnd) {
            save(sys, "end.png");
            sawEnd = true;
        }
    }
    if (shotDir && !sawEnd) save(sys, "end.png");
    std::printf("S3 LOT PURSE  %s  stalled %d/%d  score %d  (%.1f s)\n", cart.reason(), cart.stalled(), cart.fleet(),
                cart.score(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LOT PURSE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lotpurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lotp::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
