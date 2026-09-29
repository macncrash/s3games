// S3 RICKSHAW LANE
//   s3rickshawlane                 stay in the lane for the whole leg
//   s3rickshawlane --sim           autopilot holds the lane through the end
//   s3rickshawlane --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lane.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        rickshawlane::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rickshawlane::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool lane = false, edge = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!lane && m >= 1 && frames > 40) {
            save(sys, "lane.png");
            lane = true;
        } else if (!edge && m == 2) {
            save(sys, "edge.png");
            edge = true;
        } else if (!end && m == 3) {
            save(sys, "end.png");
            end = true;
        }
    }
    save(sys, "leg.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 RICKSHAW LANE  FAIL  %s  off %+.2f  along %.0f  (%.1f s)\n", why, cart.lateral(), cart.along(),
                    cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RICKSHAW LANE  HELD  stayed in the lane for the whole leg  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 RICKSHAW LANE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rickshawlane [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rickshawlane::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
