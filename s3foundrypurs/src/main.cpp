// S3 FOUNDRY PURSUIT
//   s3foundrypurs                 play
//   s3foundrypurs --sim           the furnace outlasts the other machines
//   s3foundrypurs --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/foundry.h"
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
        foundryp::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    foundryp::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool floor = false, end = false;
    int frames = 0, playFrames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 || m == 2) {
            if (++playFrames == 80 && !floor) {
                save(sys, "floor.png");
                floor = true;
            }
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && cart.stalled() == cart.fleet() && cart.heat() > 0;
    std::printf("S3 FOUNDRY PURSUIT  %s  seized %d/%d  heat %d  (%.1f s)\n",
                pass ? "THE LAST MACHINE STILL RUNNING" : cart.reason(), cart.stalled(), cart.fleet(), cart.heat(),
                frames / 60.0);
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
            std::printf("S3 FOUNDRY PURSUIT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3foundrypurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<foundryp::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
