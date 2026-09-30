// S3 BUS SLIP
//   s3busslip                 berth in the slip before the tide turns
//   s3busslip --sim           autopilot berths and the run must be a win
//   s3busslip --sim --shots D also writes PNGs into D
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

    if (shotDir) {
        gs::System sys(true);
        slip::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    slip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.x() > 120.f) {
            save(sys, "approach.png");
            mid = true;
        }
    }
    save(sys, cart.won() ? "berth.png" : "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 BUS SLIP  FAIL  %s  x %.1f  spd %.1f  tide %.1f  (%.1f s)\n", why, cart.x(), cart.speed(),
                    cart.tide(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BUS SLIP  WIN  berthed in the slip before the tide turned  (%.1f s)\n", frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BUS SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3busslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<slip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
