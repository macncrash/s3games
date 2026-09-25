// S3 DOGSLED
//   s3dogsled                  play
//   s3dogsled --sim            the team runs the checkpoint
//   s3dogsled --sim --shots D  also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sled.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        sled::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sled::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, near = false, done = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (frames < limit) {
        sys.step();
        frames++;
        const int m = cart.marker();
        if (m == 2 && !mid) {
            save(sys, "trail.png");
            mid = true;
        } else if (m == 3 && !near) {
            save(sys, "checkpoint.png");
            near = true;
        } else if (m == 4 && !done) {
            save(sys, "end.png");
            done = true;
        }
        if (cart.over()) break;
    }
    if (shotDir && !done) save(sys, "end.png");
    char clk[16];
    cart.formatElapsed(clk, int(sizeof clk));
    std::printf("S3 DOGSLED  THE CHECKPOINT  %s  gates %d/%d  %s  spills %d\n", cart.outcome(), cart.gatesTaken(),
                cart.gateCount(), clk, cart.spills());
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
            std::printf("S3 DOGSLED %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3dogsled [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sled::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
