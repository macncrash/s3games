// S3 TRAM LOCK
//   s3tramlock                 pass the lock
//   s3tramlock --sim           autopilot clears both gates ahead of the other crew
//   s3tramlock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tram.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        tramlock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tramlock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool approach = false, chamber = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!approach && m >= 1 && frames > 24) {
            save(sys, "approach.png");
            approach = true;
        } else if (!chamber && m == 2) {
            save(sys, "chamber.png");
            chamber = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 TRAM LOCK  FAIL  %s  gates %d  (%.1f s)\n", why, cart.gatesClear(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    int left = int(cart.crewLeft() + 0.5f);
    if (left < 0) left = 0;
    std::printf("S3 TRAM LOCK  PASSED  both gates clear  crew %02d:%02d left\n", left / 60, left % 60);
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
            std::printf("S3 TRAM LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tramlock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tramlock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
