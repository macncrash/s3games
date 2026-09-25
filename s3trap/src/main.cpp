// S3 TRAP
//   s3trap                 play
//   s3trap --sim           autopilot flies every landing
//   s3trap --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/trap.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        trap::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }
    gs::System sys(true);
    trap::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool approach = false, close = false, grade = false;
    int frames = 0;
    const int limit = 60 * 400;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !approach && frames > 40) {
            save(sys, "approach.png");
            approach = true;
        } else if (m == 2 && !close) {
            save(sys, "close.png");
            close = true;
        } else if (m == 3 && !grade) {
            save(sys, "grade.png");
            grade = true;
        }
    }
    if (!grade) save(sys, "grade.png");
    std::printf("S3 TRAP  %s  passes %d  (%.0f s)\n", cart.boatOk() ? "BOAT OK" : "BOAT FAILED", cart.passes(), frames / 60.0);
    return cart.boatOk() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TRAP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3trap [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<trap::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
