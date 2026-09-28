// S3 LUGE MARK
//   s3lugemark                 take the luge and set down on the mark
//   s3lugemark --sim           autopilot must beat the other crew's clock
//   s3lugemark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/luge.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        luge::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    luge::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool ice = false, mark = false, hold = false, end = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !ice && frames > 40) {
            save(sys, "ice.png");
            ice = true;
        } else if (m == 2 && !mark) {
            save(sys, "mark.png");
            mark = true;
        } else if (m == 3 && !hold) {
            save(sys, "set.png");
            hold = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("S3 LUGE MARK  %s  set down %.1fs  crew %.0f  left %.1f  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.seconds(), 18.0, cart.left(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LUGE MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lugemark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<luge::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
