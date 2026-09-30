// S3 CULVERT PURSE
//   s3culvertpurs                 play the culvert watch
//   s3culvertpurs --sim           autopilot is the last machine still running
//   s3culvertpurs --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/culvert.h"
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
        cpurs::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cpurs::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool pipe = false, last = false, end = false;
    int frames = 0;
    int playFrames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 90 && !pipe) {
                save(sys, "culvert.png");
                pipe = true;
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
    std::printf("S3 CULVERT PURSE  %s  stalled %d/%d  score %d  (%.1f s)\n", word, cart.stalled(), cart.fleet(),
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
            std::printf("S3 CULVERT PURSE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3culvertpurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cpurs::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
