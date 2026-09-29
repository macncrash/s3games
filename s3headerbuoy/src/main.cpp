// S3 HEADER BUOY
//   s3headerbuoy            play
//   s3headerbuoy --sim      autopilot sails the course against the clock
//   s3headerbuoy --sim --shots D   also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/race.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        headerbuoy::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    headerbuoy::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, end = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.buoys() >= 1) {
            save(sys, "header.png");
            mid = true;
        }
        if (!end && cart.buoys() >= 3) {
            save(sys, "buoys.png");
            end = true;
        }
    }
    if (!end) save(sys, "dock.png");
    std::printf("S3 HEADER BUOY  %s  %.1fs  crew %.1fs  buoys %d\n", cart.won() ? "PASS" : "FAIL", cart.raceTime(),
                cart.crewTime(), cart.buoys());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HEADER BUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3headerbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<headerbuoy::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
