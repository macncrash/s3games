// S3 RAIL BUOY
//   s3railbuoy                 drive the harbour loop
//   s3railbuoy --sim           autopilot rounds the buoys and takes the same dock
//   s3railbuoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/rail.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        railbuoy::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    railbuoy::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, rounding = false, home = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!out && m >= 1 && frames > 20) {
            save(sys, "rail.png");
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
        std::printf("S3 RAIL BUOY  FAIL  %s  buoys %d  s %.0f  spd %.1f  (%.1f s)\n", why, cart.buoys(), cart.along(),
                    cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RAIL BUOY  HOME  rounded the buoys and returned to the same dock  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 RAIL BUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3railbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<railbuoy::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
