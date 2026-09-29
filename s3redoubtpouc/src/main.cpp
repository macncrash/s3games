// REDOUBT POUCH
//   s3redoubtpouc            play
//   s3redoubtpouc --sim      carry the pouch across
//   s3redoubtpouc --sim --shots D   also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pouc.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        pouc::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    pouc::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool cross = false, end = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && frames == 70 && !cross) {
            save(sys, "carry.png");
            cross = true;
        } else if ((m == 2 || m == 3) && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("S3 REDOUBT POUCH  %s  the pouch is across  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 REDOUBT POUCH %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3redoubtpouc [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pouc::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
