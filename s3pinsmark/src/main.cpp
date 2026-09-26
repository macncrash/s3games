// S3 PINSMARK
//   s3pinsmark                 bowl until a mark's count closes
//   s3pinsmark --sim           autopilot finishes a mark
//   s3pinsmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pinsmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pinsmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 20) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && cart.rolling()) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.marked()) {
        if (cart.strike()) {
            std::printf("S3 PINSMARK  FINISHED MARK  strike  10+%d+%d  frame %d  (%.1f s)\n", cart.bonus(0),
                        cart.bonus(1), cart.markFrame(), frames / 60.0);
        } else {
            std::printf("S3 PINSMARK  FINISHED MARK  spare  10+%d  frame %d  (%.1f s)\n", cart.bonus(0),
                        cart.markFrame(), frames / 60.0);
        }
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PINSMARK  OPEN  no finished mark  frame %d  (%.1f s)\n", cart.markFrame(), frames / 60.0);
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
            std::printf("S3 PINSMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pinsmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pinsmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
