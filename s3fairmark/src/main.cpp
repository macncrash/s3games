// S3 FAIRMARK
//   s3fairmark                 set the coin, ring the gold, lift it
//   s3fairmark --sim           autopilot finishes the mark
//   s3fairmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fairmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    fairmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, opened = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 10) {
            save(sys, "title.png");
            titled = true;
        }
        if (!opened && cart.opened()) {
            save(sys, "open.png");
            opened = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.lifted() && cart.opened() && cart.rings() >= 1 && cart.rings() <= 3) {
        std::printf("S3 FAIRMARK  FINISHED MARK  ring on the gold bottle  coin lifted  rings %d  (%.1f s)\n",
                    cart.rings(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FAIRMARK  OPEN  no finished mark  rings %d  phase %s  (%.1f s)\n", cart.rings(), cart.phase(),
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
            std::printf("S3 FAIRMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fairmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fairmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
