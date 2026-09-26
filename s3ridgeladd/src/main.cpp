// S3 RIDGE LADD
//   s3ridgeladd                 play
//   s3ridgeladd --sim           autopilot reaches the far ladder
//   s3ridgeladd --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/ladd.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        rladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool ridge = false, brk = false, ladder = false, end = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !ridge && frames > 30) {
            save(sys, "ridge.png");
            ridge = true;
        } else if (m == 2 && !brk) {
            save(sys, "break.png");
            brk = true;
        } else if (m == 3 && !ladder) {
            save(sys, "ladder.png");
            ladder = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 RIDGE LADD  still on the ridge  %s  z %.1f  u %.2f  (%.1f s)\n", cart.reason(), cart.heroZ(),
                    cart.heroU(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RIDGE LADD  reached the far ladder\n");
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RIDGE LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgeladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
