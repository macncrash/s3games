// S3 DEPOT LADD
//   s3depotladd                 play
//   s3depotladd --sim           autopilot reaches the far ladder
//   s3depotladd --sim --shots D also writes PNGs into D
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
        depotladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 48; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depotladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool yard = false, vent = false, gantry = false, end = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!yard && m == 1 && frames > 30) {
            save(sys, "yard.png");
            yard = true;
        } else if (!vent && m == 2) {
            save(sys, "vent.png");
            vent = true;
        } else if (!gantry && m == 3) {
            save(sys, "gantry.png");
            gantry = true;
        } else if (!end && m == 4) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 DEPOT LADD  the far ladder  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DEPOT LADD  the watch is over  %s  x %.0f  y %.0f  plat %d  (%.1f s)\n",
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
            std::printf("S3 DEPOT LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depotladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
