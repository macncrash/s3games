// S3 PUTT GOLD
//   s3puttgold                 a short putt; gold counts double
//   s3puttgold --sim           autopilot holes the gold
//   s3puttgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/puttgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        puttgold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    puttgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool rolling = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!rolling && frames > 2) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.gold() == 2) {
        std::printf("S3 PUTT GOLD  DOUBLE  gold counts 2  cream %d  strokes %d  (%.1f s)\n", cart.cream(),
                    cart.strokes(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PUTT GOLD  SHORT  gold counts %d  cream %d  strokes %d  ball %.1f %.1f  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.strokes(), cart.ballX(), cart.ballY(), frames / 60.0);
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
            std::printf("S3 PUTT GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3puttgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<puttgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
