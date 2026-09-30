// S3 BELLMARK
//   s3bellmark                 pull the rope until the lip mark is finished
//   s3bellmark --sim           autopilot finishes the mark
//   s3bellmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bellmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    bellmark::Game cart;
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
    if (cart.rules() && cart.won() && cart.finished() && cart.struck() && cart.rings() == 4 && cart.pulls() == 4) {
        std::printf("S3 BELLMARK  FINISHED MARK  lip struck  rings %d  (%.1f s)\n", cart.rings(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BELLMARK  OPEN  no finished mark  rings %d  pulls %d  (%.1f s)\n", cart.rings(), cart.pulls(),
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
            std::printf("S3 BELLMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bellmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bellmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
