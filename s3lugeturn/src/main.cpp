// S3 LUGE TURN
//   s3lugeturn                 play
//   s3lugeturn --sim           autopilot holds the three bends
//   s3lugeturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/luge.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        luge::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    luge::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool a = false, b = false, c = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!a && m == 1) {
            save(sys, "turn1.png");
            a = true;
        } else if (!b && m == 2) {
            save(sys, "turn2.png");
            b = true;
        } else if (!c && m == 3) {
            save(sys, "turn3.png");
            c = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (!cart.over()) std::printf("S3 LUGE TURN  FAIL  timed out\n");
        std::fflush(stdout);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LUGE TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lugeturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<luge::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
