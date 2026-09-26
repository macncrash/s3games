// S3 EIGHTMARK
//   s3eightmark                 set the coin, call the pocket, lift the mark
//   s3eightmark --sim           autopilot finishes the mark
//   s3eightmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/eightmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    eightmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolled = false, opened = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolled && cart.rolling()) {
            save(sys, "roll.png");
            rolled = true;
        }
        if (!opened && cart.opened()) {
            save(sys, "open.png");
            opened = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.lifted() && cart.opened() && cart.shots() > 0) {
        std::printf("S3 EIGHTMARK  FINISHED MARK  8 in %s  coin lifted  shots %d  (%.1f s)\n", cart.pocketName(),
                    cart.shots(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 EIGHTMARK  OPEN  no finished mark  %s  shots %d  (%.1f s)\n", cart.reason(), cart.shots(),
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
            std::printf("S3 EIGHTMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3eightmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<eightmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
