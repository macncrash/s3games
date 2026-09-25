// S3 RAIL
//   s3rail                 drive the cab
//   s3rail --sim           autopilot makes the line
//   s3rail --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/rail.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        rail::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rail::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool rolling = false, arrived = false;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!rolling && frames == 150) {
            save(sys, "run.png");
            rolling = true;
        }
        if (!arrived && cart.stopsMade() == 1) {
            save(sys, "stop.png");
            arrived = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 RAIL  MADE THE RUN  WEST LINE  stops %d  late %d  (%.1f s)\n", cart.stopsMade(),
                    cart.lateCount(), frames / 60.0);
        return 0;
    }
    const char* why = cart.endNote()[0] ? cart.endNote() : "UNFINISHED";
    std::printf("S3 RAIL  RUN LOST  %s  stops %d  late %d  (%.1f s)\n", why, cart.stopsMade(), cart.lateCount(),
                frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RAIL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rail [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rail::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
