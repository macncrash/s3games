// S3 FOUNDRY DOOR
//   s3foundrydoor                 hold the furnace door
//   s3foundrydoor --sim           the door stays shut for three minutes
//   s3foundrydoor --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/door.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        foundrydoor::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    foundrydoor::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, end = false;
    int frames = 0;
    const int limit = 60 * 200;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 420) {
            save(sys, "foundry.png");
            mid = true;
        }
        if (cart.marker() == 2 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && frames >= 180 * 60 - 1;
    if (pass) {
        std::printf("S3 FOUNDRY DOOR  THE DOOR HELD FOR THREE MINUTES  (%.1f s)\n", frames / 60.0);
    } else {
        std::printf("S3 FOUNDRY DOOR  THE DOOR OPENED  seam %.2f  (%.1f s)\n", cart.seam(), frames / 60.0);
    }
    std::fflush(stdout);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FOUNDRY DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3foundrydoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<foundrydoor::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
