// S3 ORCHARD LADD
//   s3orchardladd                 play
//   s3orchardladd --sim           autopilot reaches the far ladder
//   s3orchardladd --sim --shots D also writes PNGs into D
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
        orchardladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 48; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    orchardladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool near = false, barrel = false, far = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!near && m == 1 && frames > 20) {
            save(sys, "orchard.png");
            near = true;
        } else if (!barrel && m == 2) {
            save(sys, "barrel.png");
            barrel = true;
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
        std::printf("S3 ORCHARD LADD  reached the far ladder  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 ORCHARD LADD  the orchard is over  %s  x %.0f  y %.0f  plat %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "STILL IN THE ORCHARD", cart.heroX(), cart.heroY(), cart.heroPlat(),
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
            std::printf("S3 ORCHARD LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3orchardladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<orchardladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
