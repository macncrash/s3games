// S3 RAIL SLIP
//   s3railslip                 play
//   s3railslip --sim           autopilot berths the three slips
//   s3railslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/slip.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        railslip::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    railslip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false, berthed = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !run) {
            save(sys, "run.png");
            run = true;
        } else if (m == 2 && !berthed) {
            save(sys, "berth.png");
            berthed = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 RAIL SLIP  FAIL  %s  slips %d  tide %.1f  (%.1f s)\n", cart.why(), cart.slips(), cart.tideLeft(),
                    frames / 60.0);
        return 1;
    }
    std::printf("S3 RAIL SLIP  BERTHED  three slips before the tide turned  (%.1f s)\n", cart.seconds());
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RAIL SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3railslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<railslip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
