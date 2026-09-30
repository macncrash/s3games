// S3 BUS BOX
//   s3busbox                 drive the leg and stop inside the box
//   s3busbox --sim           autopilot stops inside the box
//   s3busbox --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bus.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        busbox::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    busbox::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool mid = false, boxed = false;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!mid && m == 1 && frames > 50) {
            save(sys, "leg.png");
            mid = true;
        } else if (!boxed && m >= 2 && m < 4) {
            save(sys, "box.png");
            boxed = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.why();
        if (!why || !why[0]) why = "unfinished";
        std::printf("S3 BUS BOX  FAIL  %s  x %.1f spd %.2f  (%.1f s)\n", why, cart.x(), cart.speed(),
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BUS BOX  STOPPED  inside the box at the end of the leg  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 BUS BOX %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3busbox [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<busbox::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
