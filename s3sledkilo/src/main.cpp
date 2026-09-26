// S3 SLED KILO
//   s3sledkilo                 play
//   s3sledkilo --sim           autopilot finishes the kilometer ahead of the crew
//   s3sledkilo --sim --shots D also writes PNGs into D
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
        sledkilo::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sledkilo::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool away = false, wheels = false, stretch = false;
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!away && m >= 1 && frames > 28) {
            save(sys, "sled.png");
            away = true;
        } else if (!wheels && m == 2) {
            save(sys, "wheels.png");
            wheels = true;
        } else if (!stretch && m == 3) {
            save(sys, "kilo.png");
            stretch = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (!(cart.over() && cart.report()[0])) std::printf("S3 SLED KILO  FAIL  timed out\n");
        std::fflush(stdout);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SLED KILO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sledkilo [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sledkilo::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
