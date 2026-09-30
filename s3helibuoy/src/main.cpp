// S3 HELIBUOY
//   s3helibuoy                 round the buoys and dock
//   s3helibuoy --sim           autopilot beats the other crew
//   s3helibuoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/buoy.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        buoy::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    buoy::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.marks() >= 1) {
            save(sys, "round.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 HELIBUOY  DOCKED  BUOYS %d  %.1fs LEFT\n", cart.marks(), cart.left());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HELIBUOY  FAIL  %s  buoys %d  left %.1f  lives %d  (%.1fs)\n", cart.why(), cart.marks(),
                cart.left(), cart.lives(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HELIBUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3helibuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<buoy::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
