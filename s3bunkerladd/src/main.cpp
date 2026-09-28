// S3 BUNKER LADD
//   s3bunkerladd                 play
//   s3bunkerladd --sim           autopilot reaches the far ladder
//   s3bunkerladd --sim --shots D also writes PNGs into D
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
        bunkerladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 48; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    bunkerladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool bay = false, trol = false, far = false, end = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!bay && m == 1 && frames > 20) {
            save(sys, "bay.png");
            bay = true;
        } else if (!trol && m == 2) {
            save(sys, "trolley.png");
            trol = true;
        } else if (!far && m == 3) {
            save(sys, "ladder.png");
            far = true;
        } else if (!end && m == 4) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 BUNKER LADD  reached the far ladder  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BUNKER LADD  still in the bunker  %s  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "SHORT OF THE LADDER", frames / 60.0);
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
            std::printf("S3 BUNKER LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bunkerladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bunkerladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
