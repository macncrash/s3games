// S3 GRASS
//   s3grass                 fly the strip
//   s3grass --sim           autopilot flies three circuits and full-stops
//   s3grass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/grass.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        grass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    grass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool roll = false, pattern = false, final = false;
    int frames = 0;
    const int limit = 60 * 320;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!roll && cart.speed() > 16.f && cart.alt() < 1.f) {
            save(sys, "roll.png");
            roll = true;
        }
        if (!pattern && cart.pattern()) {
            save(sys, "downwind.png");
            pattern = true;
        }
        if (!final && cart.circuit() >= 3 && cart.alt() > 2.f && cart.alt() < 14.f && cart.speed() > 18.f) {
            save(sys, "final.png");
            final = true;
        }
    }
    save(sys, "end.png");
    std::printf("S3 GRASS  %s  %s  circuit %d  score %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", cart.result(),
                cart.circuit(), cart.score(), frames / 60.0);
    if (!cart.won()) {
        std::fprintf(stderr, "grass x %.1f z %.1f alt %.1f spd %.1f hdg %.2f gate %d dw %d dep %d\n", cart.flightX(),
                     cart.flightZ(), cart.alt(), cart.speed(), cart.flightHdg(), cart.gate(), cart.pattern() ? 1 : 0,
                     cart.departed() ? 1 : 0);
    }
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3grass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<grass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
