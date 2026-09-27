// S3 ARCHMARK
//   s3archmark                 set the coin, stick the gold, lift the mark
//   s3archmark --sim           autopilot finishes the mark
//   s3archmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/archmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    archmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.finished() && cart.lifted() && cart.onGold() && cart.marked() &&
        cart.arrows() >= 1 && cart.arrows() <= 3) {
        std::printf("S3 ARCHMARK  FINISHED MARK  gold arrow  lifted  arrows %d  (%.1f s)\n", cart.arrows(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 ARCHMARK  OPEN  no finished mark  arrows %d  marked %d  gold %d  lifted %d  (%.1f s)\n",
                cart.arrows(), cart.marked() ? 1 : 0, cart.onGold() ? 1 : 0, cart.lifted() ? 1 : 0, frames / 60.0);
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
            std::printf("S3 ARCHMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3archmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<archmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
