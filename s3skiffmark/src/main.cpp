// S3 SKIFF MARK
//   s3skiffmark                 take the skiff and set down on the mark
//   s3skiffmark --sim           autopilot sets down ahead of the other crew
//   s3skiffmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        skiffmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    skiffmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool reach = false, painted = false, setting = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!reach && m >= 1 && frames > 30) {
            save(sys, "reach.png");
            reach = true;
        } else if (!painted && m == 2) {
            save(sys, "mark.png");
            painted = true;
        } else if (!setting && m == 3) {
            save(sys, "set.png");
            setting = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (cart.over() && cart.report()[0]) std::printf("%s\n", cart.report());
        else
            std::printf(
                "S3 SKIFF MARK  FAIL  timed out  x %.1f  y %.1f  hdg %.0f  spd %.2f  crew %.1f  (%.1f s)\n",
                cart.x(), cart.y(), cart.heading() * 57.2958f, cart.speed(), cart.crewLeft(), frames / 60.0);
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
            std::printf("S3 SKIFF MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skiffmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skiffmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
