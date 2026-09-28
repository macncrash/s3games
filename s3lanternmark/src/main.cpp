// S3 LANTERNMARK
//   s3lanternmark                 light the order, then lift the gold mark
//   s3lanternmark --sim           autopilot finishes the mark
//   s3lanternmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lanternmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lanternmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.lifted() && cart.onMark() && cart.closer() == 3 && cart.order() == 4 &&
        cart.handed() == 0) {
        std::printf("S3 LANTERNMARK  FINISHED MARK  gold lamp closed  order %d  lifted  handed %d  (%.1f s)\n",
                    cart.order(), cart.handed(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LANTERNMARK  OPEN  no finished mark  order %d  handed %d  closer %d  (%.1f s)\n", cart.order(),
                cart.handed(), cart.closer(), frames / 60.0);
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
            std::printf("S3 LANTERNMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lanternmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lanternmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
