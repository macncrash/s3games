// S3 RICKSHAW PASS
//   s3rickshawpass                 pedal the pass before the storm
//   s3rickshawpass --sim           autopilot must clear the pass in time
//   s3rickshawpass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pass.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        pass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    pass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool climb = false, rocks = false, crest = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!climb && m == 1) {
            save(sys, "climb.png");
            climb = true;
        } else if (!rocks && m == 2) {
            save(sys, "rocks.png");
            rocks = true;
        } else if (!crest && m == 3) {
            save(sys, "crest.png");
            crest = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 RICKSHAW PASS  FAIL  %s  dist %.0f  lat %.2f  storm %.0f  (%.1f s)\n", why, cart.distance(),
                    cart.lateral(), cart.clockLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RICKSHAW PASS  CLEAR  cleared the pass before the storm  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 RICKSHAW PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rickshawpass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
