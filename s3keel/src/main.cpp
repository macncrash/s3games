// S3 KEEL
//   s3keel                 sail the triangle
//   s3keel --sim           autopilot rounds the buoys and docks
//   s3keel --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/keel.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        keel::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    keel::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 160;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 90) {
            save(sys, "sail.png");
            mid = true;
        }
    }
    save(sys, "dock.png");
    if (!cart.won()) {
        std::printf("S3 KEEL  FAIL  leg %d  wp %d  x %.0f  y %.0f  hdg %.2f  spd %.1f  (%.1fs)\n", cart.leg(),
                    cart.waypoint(), cart.x(), cart.y(), cart.heading(), cart.speed(), frames / 60.0);
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
            std::printf("S3 KEEL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3keel [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<keel::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
