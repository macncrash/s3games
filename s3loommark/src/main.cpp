// S3 LOOMMARK
//   s3loommark                 weave the mark
//   s3loommark --sim           autopilot finishes the mark
//   s3loommark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/loom.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    loommark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, woven = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!woven && cart.picks() == 4) {
            save(sys, "weave.png");
            woven = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.markWoven() && cart.picks() == 9) {
        std::printf("S3 LOOMMARK  FINISHED MARK  short loom  picks %d  mark woven  (%.1f s)\n", cart.picks(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LOOMMARK  OPEN  no finished mark  picks %d  misses %d  (%.1f s)\n", cart.picks(), cart.misses(),
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
            std::printf("S3 LOOMMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3loommark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<loommark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
