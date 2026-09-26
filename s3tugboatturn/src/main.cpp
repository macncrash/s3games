// S3 TUGBOAT TURN
//   s3tugboatturn                 make the three turns without tipping
//   s3tugboatturn --sim           autopilot must beat the other crew upright
//   s3tugboatturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/turn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        tugturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tugturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fairway = false, turn = false, berth = false;
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!fairway && m >= 1 && frames > 30) {
            save(sys, "fairway.png");
            fairway = true;
        } else if (!turn && m == 2) {
            save(sys, "turn.png");
            turn = true;
        } else if (!berth && m == 3) {
            save(sys, "berth.png");
            berth = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TUGBOAT TURN  FAIL  %s  turns %d  list %.0f  x %.1f  z %.1f  off %+.1f  hdg %.0f  spd %.1f  crew %.1f  (%.1f s)\n",
            why, cart.turns(), cart.heel() * 57.2958f, cart.x(), cart.z(), cart.lateral(), cart.heading() * 57.2958f,
            cart.speed(), cart.crewLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 TUGBOAT TURN  STEADY  made the three turns without tipping ahead of the other crew  (%.1fs, %.1fs left)\n",
        cart.seconds(), cart.crewLeft());
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
            std::printf("S3 TUGBOAT TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tugturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
