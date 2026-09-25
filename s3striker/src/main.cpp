// S3 STRIKER
//   s3striker                 play
//   s3striker --sim           three swings, ring the bell
//   s3striker --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/striker.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        striker::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 50; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    striker::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool air = false, bell = false, end = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 2 && !air) {
            save(sys, "swing.png");
            air = true;
        } else if (m == 3 && !bell) {
            save(sys, "bell.png");
            bell = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("S3 STRIKER  %s  swing %d of 3  best %d  (%.1f s)\n", cart.won() ? "RING" : "QUIET",
                cart.won() ? cart.swing() : 3, cart.best(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 STRIKER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3striker [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<striker::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
