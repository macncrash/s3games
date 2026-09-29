// S3 MUSH SLIP
//   s3mushslip                 berth the mush
//   s3mushslip --sim           autopilot berths before the tide turns
//   s3mushslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mush.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        mushslip::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mushslip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool ice = false, slip = false, held = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!ice && m >= 1 && frames > 24) {
            save(sys, "mush.png");
            ice = true;
        } else if (!slip && m == 2) {
            save(sys, "slip.png");
            slip = true;
        } else if (!held && m == 3) {
            save(sys, "berth.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = (cart.over() && cart.why()[0]) ? cart.why() : "timed out";
        std::printf(
            "S3 MUSH SLIP  FAIL  %s  x %.1f y %.1f hdg %.2f spd %.2f tide %.1f slip %d  (%.1f s)\n", why, cart.x(),
            cart.y(), cart.heading(), cart.speed(), cart.tideLeft(), cart.inSlip() ? 1 : 0, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 MUSH SLIP  BERTHED  berthed in the slip before the tide turned  (%.1f s, %.1f s of tide left)\n",
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
            std::printf("S3 MUSH SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mushslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mushslip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
