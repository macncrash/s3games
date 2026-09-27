// S3 SPANWELL
//   s3spanwell                 keep the well standing
//   s3spanwell --sim           autopilot holds the span through three waves
//   s3spanwell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/spanwell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        spanwell::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    spanwell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool surge = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!surge && cart.surged()) {
            save(sys, "wave.png");
            surge = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.wave() >= 2) {
        std::printf("S3 SPANWELL  STOOD  the well held the span through wave 3  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SPANWELL  FELL  the well left the span on wave %d  (%.1f s)\n", cart.wave() + 1, frames / 60.0);
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
            std::printf("S3 SPANWELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3spanwell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<spanwell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
