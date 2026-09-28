// S3 SAFEMARK
//   s3safemark                 match the room, pull the gold mark
//   s3safemark --sim           autopilot finishes the mark
//   s3safemark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/safe.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System titleSys(true);
        safemark::Game title;
        titleSys.bootCart(title);
        for (int i = 0; i < 5; i++) titleSys.step();
        save(titleSys, "title.png");
    }

    gs::System sys(true);
    safemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.marked() && cart.solved() && cart.gold() >= 0 && cart.gold() < 3 &&
        cart.dial(cart.gold()) == cart.combo(cart.gold())) {
        std::printf("S3 SAFEMARK  FINISHED MARK  gold %d  %d-%d-%d  (%.1f s)\n", cart.gold(), cart.combo(0),
                    cart.combo(1), cart.combo(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SAFEMARK  OPEN  no finished mark  (%.1f s)\n", frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SAFEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3safemark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<safemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
