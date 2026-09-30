// S3 TRENCH LADD
//   s3trenchladd                 play
//   s3trenchladd --sim           autopilot climbs the far ladder
//   s3trenchladd --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ladd.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        trenchladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    trenchladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false, gun = false, end = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !run && frames > 30) {
            save(sys, "trench.png");
            run = true;
        } else if (m == 2 && !gun) {
            save(sys, "gun.png");
            gun = true;
        } else if (m >= 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 TRENCH LADD  the trench kept you  %s  lives %d  x %.0f  climb %.2f  watch %.1f  (%.1f s)\n",
                    cart.reason(), cart.lives(), cart.heroX(), cart.climb(), cart.watchLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TRENCH LADD  reached the far ladder\n");
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
            std::printf("S3 TRENCH LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3trenchladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<trenchladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
