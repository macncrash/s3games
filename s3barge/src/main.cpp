// S3 BARGE
//   s3barge                 take the lock
//   s3barge --sim           autopilot must rise and leave without a scrape
//   s3barge --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/barge.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        barge::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    barge::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.phase() == 2) {
            save(sys, "rise.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 BARGE  FAIL  %s  x %.1f  lat %.2f  spd %.2f  water %.2f  phase %d  (%.1f s)\n", why, cart.x(),
                    cart.lateral(), cart.speed(), cart.risen(), cart.phase(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BARGE  CLEAR  rose %.1f m and left the lock without a scrape  (%.1f s)\n", cart.risen(),
                frames / 60.0);
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
            std::printf("S3 BARGE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3barge [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<barge::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
