// S3 BUS PASS
//   s3buspass                 drive the pass
//   s3buspass --sim           autopilot must clear the pass ahead of the other crew
//   s3buspass --sim --shots D also writes PNGs into D
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
        buspass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    buspass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool climb = false, gate = false, shelf = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!climb && m == 1) {
            save(sys, "climb.png");
            climb = true;
        } else if (!gate && m == 2) {
            save(sys, "gate.png");
            gate = true;
        } else if (!shelf && m == 3) {
            save(sys, "shelf.png");
            shelf = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 BUS PASS  FAIL  %s  (%.1f s)\n", why, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BUS PASS  CLEARED  the pass before the storm clock  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 BUS PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3buspass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<buspass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
