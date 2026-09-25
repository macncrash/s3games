// S3 METRO
//   s3metro                 play
//   s3metro --sim           autopilot must stop in every box
//   s3metro --sim --shots D also writes PNGs into D
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
    bool runShot = false, dockShot = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!runShot && cart.marker() == 1) {
            save(sys, "run.png");
            runShot = true;
        } else if (!dockShot && cart.marker() == 2) {
            save(sys, "dock.png");
            dockShot = true;
        }
    }
    save(sys, "end.png");
    std::string line = cart.report();
    std::printf("%s\n", line.c_str());
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 METRO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3metro [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<metro::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
