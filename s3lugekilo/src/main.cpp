// S3 LUGE KILO
//   s3lugekilo                 play
//   s3lugekilo --sim           autopilot finishes the kilometer with the wheels clear
//   s3lugekilo --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/luge.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        luge::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    luge::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool ice = false, wheel = false, stretch = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!ice && m >= 1 && frames > 20) {
            save(sys, "ice.png");
            ice = true;
        } else if (!wheel && m == 2) {
            save(sys, "wheel.png");
            wheel = true;
        } else if (!stretch && m == 3) {
            save(sys, "kilo.png");
            stretch = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (!cart.over()) std::printf("S3 LUGE KILO  FAIL  timed out\n");
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
            std::printf("S3 LUGE KILO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lugekilo [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<luge::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
