// S3 SPAN MAGA
//   s3spanmaga                 play the span
//   s3spanmaga --sim           autopilot makes the magazine outlast the raid
//   s3spanmaga --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/span.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        spanmaga::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    spanmaga::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool deck = false, hold = false, end = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        if (!deck && cart.raidTime() > 6.f) {
            save(sys, "span.png");
            deck = true;
        } else if (!hold && cart.raidTime() > 18.f) {
            save(sys, "hold.png");
            hold = true;
        }
        if (cart.over() && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SPAN MAGA  THE MAGAZINE OUTLASTS THE RAID  rounds %d  stopped %d  (%.1f s)\n", cart.rounds(),
                    cart.stopped(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SPAN MAGA  FAIL  %s  rounds %d  stopped %d  (%.1f s)\n", cart.result(), cart.rounds(),
                cart.stopped(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SPAN MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3spanmaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<spanmaga::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
