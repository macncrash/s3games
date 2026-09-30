// S3 RAILBOOM
//   s3railboom                 play
//   s3railboom --sim           autopilot delivers the drive
//   s3railboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/rail.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        rail::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rail::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!run && frames == 80) {
            save(sys, "rail.png");
            run = true;
        }
    }
    if (!end) save(sys, "boom.png");
    if (cart.won()) {
        std::printf("S3 RAILBOOM  PASS  drive on the boom  crew %.1fs behind  (%.1f s)\n", cart.crewLeft(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 RAILBOOM  FAIL  crew clock %.1f  (%.1f s)\n", cart.clock(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RAILBOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3railboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rail::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
