// S3 SPAN POUC
//   s3spanpouc                 play
//   s3spanpouc --sim           autopilot carries the pouch across
//   s3spanpouc --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/pouc.h"
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
        spanpouc::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    spanpouc::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool carry = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !carry && frames > 40) {
            save(sys, "carry.png");
            carry = true;
        } else if (m >= 2 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SPAN POUC  the pouch crossed the span\n");
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SPAN POUC  not done  %s  (%.1f s)\n", cart.reason(), frames / 60.0);
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
            std::printf("S3 SPAN POUC %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3spanpouc [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<spanpouc::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
