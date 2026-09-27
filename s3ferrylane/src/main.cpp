// S3 FERRY LANE
//   s3ferrylane                 stay in the lane for the leg
//   s3ferrylane --sim           autopilot keeps the channel and beats the crew
//   s3ferrylane --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ferry.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        ferrylane::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    ferrylane::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool cross = false, mid = false, slip = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!cross && m >= 1 && frames > 20) {
            save(sys, "cross.png");
            cross = true;
        } else if (!mid && m == 2) {
            save(sys, "lane.png");
            mid = true;
        } else if (!slip && m == 3) {
            save(sys, "slip.png");
            slip = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 FERRY LANE  FAIL  %s  z %.1f  lat %.2f  spd %.1f  margin %.2f  crew %.1f  (%.1f s)\n",
            why, cart.z(), cart.lateral(), cart.speed(), cart.margin(), cart.crewLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 FERRY LANE  PASS  stayed in the lane for the whole leg  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 FERRY LANE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ferrylane [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ferrylane::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
