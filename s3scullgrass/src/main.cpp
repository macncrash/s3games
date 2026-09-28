// S3 SCULL GRASS
//   s3scullgrass                 land the scull on the grass
//   s3scullgrass --sim           autopilot beaches and full-stops
//   s3scullgrass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scull.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    scullgrass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 50;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 50) save(sys, "row.png");
        if (shotDir && !mid && frames > 120) {
            save(sys, "grass.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "stop.png");
    if (!cart.won()) {
        std::printf("S3 SCULL GRASS  FAIL  %s  (%.1f s)\n", cart.over() ? cart.why() : "timed out", frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SCULL GRASS  PASS  landed on the grass and came to a full stop  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SCULL GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullgrass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scullgrass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
