// S3 METROBOOM
//   s3metroboom                 play
//   s3metroboom --sim           autopilot delivers the drive
//   s3metroboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/metro.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        metro::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    metro::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!run && frames == 80) {
            save(sys, "metro.png");
            run = true;
        }
    }
    save(sys, "boom.png");
    if (cart.won()) {
        std::printf("S3 METROBOOM  PASS  drive on the boom  crew %.1fs behind  (%.1f s)\n", cart.crewLeft(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 METROBOOM  FAIL  crew clock %.1f  (%.1f s)\n", cart.clock(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 METROBOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3metroboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<metro::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
