// S3 BARGE PASS
//   s3bargepass                 play
//   s3bargepass --sim           autopilot must clear the pass
//   s3bargepass --sim --shots D also writes PNGs into D
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
        bargepass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    bargepass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool wide = false, pinch = false, mouth = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!wide && m == 1 && frames > 30) {
            save(sys, "wide.png");
            wide = true;
        } else if (!pinch && m == 2) {
            save(sys, "pinch.png");
            pinch = true;
        } else if (!mouth && m == 3) {
            save(sys, "mouth.png");
            mouth = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 BARGE PASS  FAIL  %s  (%.1f s)\n", why, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BARGE PASS  CLEAR  cleared the pass before the storm clock  (%.1f s, storm had %.1f s left)\n",
                cart.seconds(), cart.stormLeft());
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
            std::printf("S3 BARGE PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bargepass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bargepass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
