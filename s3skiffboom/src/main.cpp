// S3 SKIFF BOOM
//   s3skiffboom                 take the skiff
//   s3skiffboom --sim           autopilot delivers the drive ahead of the crew
//   s3skiffboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/skiff.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        skiffboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    skiffboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool reach = false, pocket = false, held = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!reach && m >= 1 && frames > 30) {
            save(sys, "reach.png");
            reach = true;
        } else if (!pocket && m == 2) {
            save(sys, "pocket.png");
            pocket = true;
        } else if (!held && m == 3) {
            save(sys, "hold.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (cart.over() && cart.report()[0]) std::printf("%s\n", cart.report());
        else std::printf("S3 SKIFF BOOM  FAIL  timed out  x %.1f  y %.1f  hdg %.0f  spd %.2f\n", cart.x(), cart.y(),
                         cart.heading() * 57.2958f, cart.speed());
        std::fflush(stdout);
        return 1;
    }
    std::printf("%s\n", cart.report());
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
            std::printf("S3 SKIFF BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skiffboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skiffboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
