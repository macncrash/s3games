// S3 GATE LADD
//   s3gateladd                 play
//   s3gateladd --sim           autopilot reaches the far ladder
//   s3gateladd --sim --shots D also writes PNGs into D
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
        gateladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 48; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gateladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool wall = false, far = false, end = false;
    int frames = 0;
    const int limit = 60 * 55;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!wall && m == 1 && frames > 40) {
            save(sys, "gate.png");
            wall = true;
        } else if (!far && m == 2) {
            save(sys, "far.png");
            far = true;
        } else if (!end && m == 3) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 GATE LADD  THE FAR LADDER  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 GATE LADD  %s  x %.0f  y %.0f  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "STILL ON THE WALL", cart.heroX(), cart.heroY(), frames / 60.0);
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
            std::printf("S3 GATE LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gateladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gateladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
