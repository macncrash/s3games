// S3 FOUNDRY WELL
//   s3foundrywell                 play
//   s3foundrywell --sim           the foundry keeps the well through three waves
//   s3foundrywell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/foundry.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        foundrywell::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    foundrywell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool floor = false, end = false;
    int frames = 0, playFrames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 140 && !floor) {
                save(sys, "foundry.png");
                floor = true;
            }
        } else if (m >= 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && cart.courses() > 0 && cart.wave() == 2;
    std::printf("S3 FOUNDRY WELL  %s  courses %d  breaches %d  wave %d  score %d  (%.1f s)\n",
                pass ? "THE WELL STANDS" : cart.reason(), cart.courses(), cart.breaches(), pass ? 3 : cart.wave() + 1,
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
            std::printf("S3 FOUNDRY WELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3foundrywell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<foundrywell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
