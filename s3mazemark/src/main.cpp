// S3 MAZEMARK
//   s3mazemark                 walk the hedge, lift the coin
//   s3mazemark --sim           autopilot finishes the mark
//   s3mazemark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mazemark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        mazemark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 16; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mazemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool walked = false, marked = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!walked && cart.steps() == 4) {
            save(sys, "walk.png");
            walked = true;
        }
        if (!marked && cart.onMark()) {
            save(sys, "mark.png");
            marked = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.lifted() && cart.steps() > 0) {
        std::printf("S3 MAZEMARK  FINISHED MARK  on the mark  coin lifted  steps %d  (%.1f s)\n", cart.steps(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MAZEMARK  OPEN  no finished mark  steps %d  gates %d  (%.1f s)\n", cart.steps(), cart.leaves(),
                frames / 60.0);
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
            std::printf("S3 MAZEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mazemark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mazemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
