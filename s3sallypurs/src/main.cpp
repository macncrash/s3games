// S3 SALLY
//   s3sallypurs                 play
//   s3sallypurs --sim           autopilot is the last machine still running
//   s3sallypurs --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sally.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        sally::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sally::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawRun = false, sawStall = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawRun && frames > 70) {
            save(sys, "sally.png");
            sawRun = true;
        } else if (m == 2 && !sawStall) {
            save(sys, "stall.png");
            sawStall = true;
        } else if (m == 3 && !sawEnd) {
            save(sys, "end.png");
            sawEnd = true;
        }
    }
    if (!sawEnd) save(sys, "end.png");
    const char* word = cart.won() ? "THE LAST MACHINE STILL RUNNING" : cart.reason();
    std::printf("S3 SALLY  %s  score %d  (%.1f s)\n", word, cart.score(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SALLY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sallypurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sally::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
