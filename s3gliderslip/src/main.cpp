// S3 GLIDER SLIP
//   s3gliderslip                 berth in the slip
//   s3gliderslip --sim           autopilot berths before the tide turns
//   s3gliderslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/slip.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        gslip::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gslip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool glide = false, slip = false, berth = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!glide && m >= 1 && frames > 24) {
            save(sys, "glide.png");
            glide = true;
        } else if (!slip && m == 2) {
            save(sys, "slip.png");
            slip = true;
        } else if (!berth && m == 3) {
            save(sys, "berth.png");
            berth = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 GLIDER SLIP  FAIL  %s  x %.1f  alt %.1f  spd %.1f  vs %.2f  tide %.1f  afloat %d  (%.1f s)\n",
            why, cart.x(), cart.alt(), cart.speed(), cart.vs(), cart.crewLeft(), cart.afloat() ? 1 : 0,
            frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GLIDER SLIP  PASS  berthed in the slip before the tide turned  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 GLIDER SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gliderslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gslip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
