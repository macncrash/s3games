// S3 AMBER
//   s3amber                 one intersection, you keep the lamp
//   s3amber --sim           autopilot whistles only the cars that run it
//   s3amber --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/amber.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        amber::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    amber::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool play = false, end = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!play && m == 1 && frames > 90) {
            save(sys, "shift.png");
            play = true;
        } else if (!end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 AMBER  THE BOX IS CLEAR  stopped %d  spared %d  score %d\n", cart.stopped(), cart.spared(),
                    cart.score());
        return 0;
    }
    std::printf("S3 AMBER  THE LIGHT WINS  stopped %d  spared %d  score %d\n", cart.stopped(), cart.spared(),
                cart.score());
    std::fprintf(stderr, "fail lives %d over %d frames %d\n", cart.lives(), cart.over() ? 1 : 0, frames);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 AMBER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3amber [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<amber::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
