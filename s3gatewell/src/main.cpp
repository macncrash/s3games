// S3 GATE WELL
//   s3gatewell                 play
//   s3gatewell --sim           the gate keeps the well through three waves
//   s3gatewell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/well.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        well::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    well::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false, end = false;
    int frames = 0, playFrames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 160 && !fight) {
                save(sys, "gate.png");
                fight = true;
            }
        } else if (m >= 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && cart.stones() > 0 && cart.wave() == 2 && cart.through() == 0;
    std::printf("S3 GATE WELL  %s  stones %d  through %d  wave %d  score %d  (%.1f s)\n",
                pass ? "THE WELL STANDS" : cart.reason(), cart.stones(), cart.through(), cart.wave() + 1,
                cart.score(), frames / 60.0);
    std::fflush(stdout);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GATE WELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gatewell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<well::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
