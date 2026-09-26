// S3 SKIFF BUOY
//   s3skiffbuoy                 run the harbor
//   s3skiffbuoy --sim           autopilot rounds the buoys and takes the end
//   s3skiffbuoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/skiff.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        skiff::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    skiff::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, rounding = false, end = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!out && m >= 1 && frames > 20) {
            save(sys, "skiff.png");
            out = true;
        } else if (!rounding && m == 2) {
            save(sys, "buoy.png");
            rounding = true;
        } else if (!end && m == 3) {
            save(sys, "end.png");
            end = true;
        }
    }
    save(sys, "dock.png");
    if (!cart.won()) {
        std::printf("S3 SKIFF BUOY  FAIL  %s  leg %d  x %.1f  y %.1f  hdg %.2f  spd %.1f  round %.2f  (%.1fs)\n",
                    cart.over() ? "missed the end" : "timed out", cart.leg(), cart.x(), cart.y(), cart.heading(),
                    cart.speed(), cart.roundProg(), frames / 60.0);
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
            std::printf("S3 SKIFF BUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skiffbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skiff::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
