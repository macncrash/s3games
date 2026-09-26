// S3 YARD LADD
//   s3yardladd                 play
//   s3yardladd --sim           autopilot reaches the far ladder
//   s3yardladd --sim --shots D also writes PNGs into D
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
        yardladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 48; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    yardladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool yard = false, chain = false, far = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!yard && m == 1 && frames > 24) {
            save(sys, "yard.png");
            yard = true;
        } else if (!chain && m == 2) {
            save(sys, "chain.png");
            chain = true;
        } else if (!far && m == 3) {
            save(sys, "far.png");
            far = true;
        } else if (!end && m == 4) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 YARD LADD  reached the far ladder  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 YARD LADD  the watch is over  %s  x %.0f  y %.0f  plat %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "STILL IN THE YARD", cart.heroX(), cart.heroY(), cart.heroPlat(),
                frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 YARD LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3yardladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<yardladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
