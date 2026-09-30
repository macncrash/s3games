// S3 CULVERT POUC
//   s3culvertpouc                 play
//   s3culvertpouc --sim           autopilot carries the pouch across
//   s3culvertpouc --sim --shots D also writes PNGs into D
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
        culvertpouc::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    culvertpouc::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool throat = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!throat && cart.carrying() && cart.heroX() > 700.f) {
            save(sys, "culvert.png");
            throat = true;
        }
        if (cart.marker() >= 2 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 CULVERT POUC  THE POUCH CROSSED  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CULVERT POUC  THE WATCH IS OVER  (%.1f s)\n", frames / 60.0);
    std::fflush(stdout);
    cart.dump("sim");
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CULVERT POUC %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3culvertpouc [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<culvertpouc::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
