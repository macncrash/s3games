// S3 STRIKERMARK
//   s3strikermark                 swing until the gold mark is finished
//   s3strikermark --sim           autopilot finishes the mark
//   s3strikermark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/striker.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    strikermark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.finished() && cart.gold() && cart.mark() == 4 && cart.swings() >= 1 &&
        cart.swings() <= 3) {
        std::printf("S3 STRIKERMARK  FINISHED MARK  gold bell  swing %d of 3  (%.1f s)\n", cart.swings(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 STRIKERMARK  OPEN  no finished mark  swing %d  mark %d  (%.1f s)\n", cart.swings(), cart.mark(),
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
            std::printf("S3 STRIKERMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3strikermark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<strikermark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
