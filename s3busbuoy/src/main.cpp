// S3 BUSBUOY
//   s3busbuoy                 round the buoys back to the dock
//   s3busbuoy --sim           autopilot sails the course
//   s3busbuoy --sim --shots D also writes PNGs into D
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
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    buoy::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool mid = false;
    const int limit = 60 * 130;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.rounded() >= 2) {
            save(sys, "run.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    const char* line = cart.report();
    if (!line || !line[0]) {
        std::printf("S3 BUSBUOY  FAIL  unfinished  rounded %d/3  (%.1f s)\n", cart.rounded(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("%s\n", line);
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BUSBUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3busbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<buoy::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
