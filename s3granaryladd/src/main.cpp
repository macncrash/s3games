// S3 GRANARY LADD
//   s3granaryladd                 play
//   s3granaryladd --sim           autopilot reaches the far ladder
//   s3granaryladd --sim --shots D also writes PNGs into D
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
        granaryladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    granaryladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool bay = false, barrel = false, loft = false, end = false;
    int frames = 0;
    const int limit = 60 * 55;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!bay && m == 1 && frames > 20) {
            save(sys, "bay.png");
            bay = true;
        } else if (!barrel && m == 2) {
            save(sys, "barrel.png");
            barrel = true;
        } else if (!loft && m == 3) {
            save(sys, "loft.png");
            loft = true;
        } else if (!end && m == 4) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 GRANARY LADD  reached the far ladder  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 GRANARY LADD  the granary is over  %s  x %.0f  y %.0f  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "STILL IN THE GRANARY", cart.heroX(), cart.heroY(), frames / 60.0);
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
            std::printf("S3 GRANARY LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3granaryladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<granaryladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
