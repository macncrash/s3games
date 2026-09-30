// S3 METROBOX
//   s3metrobox                 play
//   s3metrobox --sim           autopilot stops inside every box
//   s3metrobox --sim --shots D also writes PNGs into D
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
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    metro::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    bool shotRun = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!shotRun && frames == 70) {
            save(sys, "run.png");
            shotRun = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    std::printf("%s\n", cart.report()[0] ? cart.report() : "S3 METROBOX  FAIL  unfinished");
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 METROBOX %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3metrobox [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<metro::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
