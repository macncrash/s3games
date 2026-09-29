// S3 RICKSHAW SLIP
//   s3rickshawslip                 berth the rickshaw
//   s3rickshawslip --sim           autopilot berths before the tide turns
//   s3rickshawslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/rick.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        rickslip::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rickslip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool quay = false, slip = false, end = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!quay && m >= 1 && frames > 30) {
            save(sys, "quay.png");
            quay = true;
        } else if (!slip && m == 2) {
            save(sys, "slip.png");
            slip = true;
        } else if (!end && m == 3) {
            save(sys, "end.png");
            end = true;
        }
    }
    save(sys, "berth.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 RICKSHAW SLIP  FAIL  %s  x %.1f  y %.1f  hdg %.0f  spd %.2f  tide %.1f  slip %d  end %d  line %d  (%.1f s)\n",
            why, cart.x(), cart.y(), cart.heading() * 57.2958f, cart.speed(), cart.tideLeft(), cart.inSlip() ? 1 : 0,
            cart.inEnd() ? 1 : 0, cart.lined() ? 1 : 0, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RICKSHAW SLIP  BERTHED  berthed in the slip before the tide turned  (%.1f s, %.1f s of tide left)\n",
                cart.seconds(), cart.tideLeft());
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
            std::printf("S3 RICKSHAW SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rickshawslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rickslip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
