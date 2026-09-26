// S3 SLED BOOM
//   s3sledboom                 take the sled
//   s3sledboom --sim           autopilot delivers the drive ahead of the crew
//   s3sledboom --sim --shots D also writes PNGs into D
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
        sledboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sledboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool tongue = false, notch = false, held = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!tongue && m >= 1 && frames > 30) {
            save(sys, "tongue.png");
            tongue = true;
        } else if (!notch && m == 2) {
            save(sys, "notch.png");
            notch = true;
        } else if (!held && m == 3) {
            save(sys, "hold.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (cart.over() && cart.report()[0]) std::printf("%s\n", cart.report());
        else
            std::printf("S3 SLED BOOM  FAIL  timed out  x %.1f  y %.1f  hdg %.0f  spd %.2f  crew %.1f\n", cart.x(),
                        cart.y(), cart.heading() * 57.2958f, cart.speed(), cart.crewLeft());
        std::fflush(stdout);
        return 1;
    }
    std::printf("%s\n", cart.report());
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
            std::printf("S3 SLED BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sledboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sledboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
