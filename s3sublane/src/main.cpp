// S3 SUB LANE
//   s3sublane                 the sub stays in the lane
//   s3sublane --sim           autopilot holds the channel for the leg
//   s3sublane --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sub.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        sublane::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sublane::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool lane = false, edge = false, dock = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!lane && m >= 1 && frames > 36) {
            save(sys, "lane.png");
            lane = true;
        } else if (!edge && m == 2) {
            save(sys, "edge.png");
            edge = true;
        } else if (!dock && m == 3) {
            save(sys, "dock.png");
            dock = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 SUB LANE  FAIL  %s  off %+.2f  (%.1f s)\n", why, cart.lateral(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SUB LANE  HELD  stayed in the lane for the whole leg  (%.1f s)\n", cart.seconds());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SUB LANE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sublane [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sublane::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
