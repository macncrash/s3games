// S3 SCULLMARK
//   s3scullmark                 play
//   s3scullmark --sim           autopilot sets the scull down on the mark
//   s3scullmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scull.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        scull::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    scull::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool row = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m >= 1 && m < 4 && !row && frames > 50) {
            save(sys, "leg.png");
            row = true;
        }
        if (m == 4 && !end) {
            save(sys, "win.png");
            end = true;
        }
    }
    if (!end) save(sys, "win.png");
    std::printf("S3 SCULLMARK  %s  set down on the mark  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SCULLMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scull::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
