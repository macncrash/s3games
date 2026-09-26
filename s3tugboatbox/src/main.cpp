// S3 TUGBOAT BOX
//   s3tugboatbox                 sail the leg
//   s3tugboatbox --sim           autopilot stops inside the box
//   s3tugboatbox --sim --shots D also writes PNGs into D
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
        tugbox::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tugbox::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool way = false, boxed = false, held = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!way && m >= 1 && frames > 30) {
            save(sys, "tug.png");
            way = true;
        } else if (!boxed && m == 2) {
            save(sys, "box.png");
            boxed = true;
        } else if (!held && m == 3) {
            save(sys, "hold.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 TUGBOAT BOX  FAIL  %s  x %.1f y %.1f hdg %.2f spd %.2f eng %.2f phase %d  (%.1f s)\n", why,
                    cart.x(), cart.y(), cart.heading(), cart.speed(), cart.engine(), cart.phase(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TUGBOAT BOX  STOPPED  inside the box  the leg is made  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 TUGBOAT BOX %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatbox [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tugbox::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
