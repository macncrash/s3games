// S3 BUNKER
//   s3bunker                 one room, one door
//   s3bunker --sim           autopilot holds the room
//   s3bunker --sim --shots D also writes PNGs into D
//
//   Arrows aim the slit. Z or C fires. X or Space braces the bar. Enter starts.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "../version.h"
#include "bunker.h"
#include "console/system.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        bunker::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 16; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    bunker::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false, end = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (shotDir && !fight && m == 1 && frames == 140) {
            save(sys, "slit.png");
            fight = true;
        } else if (shotDir && !end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 BUNKER  THE ROOM HELD  score %d\n", cart.score());
        return 0;
    }
    std::printf("S3 BUNKER  THE DOOR OPENS  score %d\n", cart.score());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BUNKER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bunker [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bunker::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
