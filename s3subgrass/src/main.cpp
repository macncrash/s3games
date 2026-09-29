// S3 SUB GRASS
//   s3subgrass                 play
//   s3subgrass --sim           autopilot lands and comes to a full stop
//   s3subgrass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sub.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        subgrass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    subgrass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false, shelf = false, hold = false, end = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !run && frames > 40) {
            save(sys, "underway.png");
            run = true;
        } else if (m == 2 && !shelf) {
            save(sys, "grass.png");
            shelf = true;
        } else if (m == 3 && !hold) {
            save(sys, "hold.png");
            hold = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 SUB GRASS  FAIL  %s  (%.1f s)\n", "did not stop on the grass", frames / 60.0);
        return 1;
    }
    std::printf("S3 SUB GRASS  PASS  landed on the grass and came to a full stop  (%.1f s)\n", cart.seconds());
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SUB GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3subgrass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<subgrass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
