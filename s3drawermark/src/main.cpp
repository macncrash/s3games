// S3 DRAWERMARK
//   s3drawermark                 seat the gold and shut the drawer
//   s3drawermark --sim           autopilot finishes the mark
//   s3drawermark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/drawermark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    drawermark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    bool seated = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!seated && cart.onMark()) {
            save(sys, "mark.png");
            seated = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.shut() && cart.onMark() && cart.cents() == cart.mark()) {
        std::printf("S3 DRAWERMARK  FINISHED MARK  gold seated  $%d.%02d  drawer shut  (%.1f s)\n",
                    cart.cents() / 100, cart.cents() % 100, frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DRAWERMARK  OPEN  no finished mark  drawer %d  (%.1f s)\n", cart.cents(), frames / 60.0);
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
            std::printf("S3 DRAWERMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3drawermark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<drawermark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
