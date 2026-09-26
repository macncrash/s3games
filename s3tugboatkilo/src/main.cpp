// S3 TUGBOAT KILO
//   s3tugboatkilo                 sail the leg
//   s3tugboatkilo --sim           autopilot finishes the kilometer
//   s3tugboatkilo --sim --shots D also writes PNGs into D
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
        tugkilo::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tugkilo::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool away = false, wheels = false, stretch = false;
    int frames = 0;
    const int limit = 60 * 170;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!away && m >= 1 && frames > 30) {
            save(sys, "tug.png");
            away = true;
        } else if (!wheels && m == 2) {
            save(sys, "wheel.png");
            wheels = true;
        } else if (!stretch && m == 3) {
            save(sys, "kilo.png");
            stretch = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (!cart.over()) {
            std::printf("S3 TUGBOAT KILO  FAIL  timed out\n");
            std::fflush(stdout);
        }
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
            std::printf("S3 TUGBOAT KILO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatkilo [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tugkilo::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
