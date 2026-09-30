// S3 GRANARY PURSE
//   s3granarypurs                 play the granary watch
//   s3granarypurs --sim           autopilot is the last machine still running
//   s3granarypurs --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/granary.h"
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
        gpurs::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gpurs::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool yard = false, last = false, end = false;
    int frames = 0;
    int playFrames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 90 && !yard) {
                save(sys, "yard.png");
                yard = true;
            }
        } else if (m == 2 && !last) {
            save(sys, "last.png");
            last = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    const char* word = cart.won() ? "THE LAST MACHINE STILL RUNNING" : "THE WATCH IS OVER";
    std::printf("S3 GRANARY PURSE  %s  stalled %d/%d  score %d  (%.1f s)\n", word, cart.stalled(), cart.fleet(),
                cart.score(), frames / 60.0);
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GRANARY PURSE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3granarypurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gpurs::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
