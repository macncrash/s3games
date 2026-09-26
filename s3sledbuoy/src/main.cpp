// S3 SLED BUOY
//   s3sledbuoy                 mush the frozen harbor
//   s3sledbuoy --sim           autopilot rounds the buoys and takes the same dock
//   s3sledbuoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sled.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        sled::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sled::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, rounding = false, home = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!out && m >= 1 && frames > 24) {
            save(sys, "sled.png");
            out = true;
        } else if (!rounding && m == 2) {
            save(sys, "buoy.png");
            rounding = true;
        } else if (!home && m == 3) {
            save(sys, "dock.png");
            home = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = (cart.over() && cart.why()[0]) ? cart.why() : "timed out";
        std::printf(
            "S3 SLED BUOY  FAIL  %s  leg %d  wp %d  x %.0f  y %.0f  hdg %.2f  spd %.1f  (%.1f s)\n", why, cart.leg(),
            cart.waypoint(), cart.x(), cart.y(), cart.heading(), cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SLED BUOY  HOME  rounded the buoys and returned to the same dock  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SLED BUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sledbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sled::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
