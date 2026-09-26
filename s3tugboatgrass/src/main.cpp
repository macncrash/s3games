// S3 TUGBOAT GRASS
//   s3tugboatgrass                 land on the grass
//   s3tugboatgrass --sim           autopilot lands and comes to a full stop
//   s3tugboatgrass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tug.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        tuggrass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tuggrass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool water = false, grass = false, held = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!water && m >= 1 && frames > 30) {
            save(sys, "tug.png");
            water = true;
        } else if (!grass && m == 2) {
            save(sys, "grass.png");
            grass = true;
        } else if (!held && m == 3) {
            save(sys, "stop.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TUGBOAT GRASS  FAIL  %s  x %.1f y %.1f hdg %.2f spd %.2f eng %.2f grass %d phase %d  (%.1f s)\n",
            why, cart.x(), cart.y(), cart.heading(), cart.speed(), cart.engine(), cart.onGrass() ? 1 : 0, cart.phase(),
            cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TUGBOAT GRASS  PASS  landed on the grass and came to a full stop  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 TUGBOAT GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatgrass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tuggrass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
