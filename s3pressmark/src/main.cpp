// S3 PRESSMARK
//   s3pressmark                 pull until the mark is finished
//   s3pressmark --sim           autopilot finishes the mark
//   s3pressmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/press.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pressmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 12) save(sys, "title.png");
        if (frames == 90) save(sys, "press.png");
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.pulls() >= 1 && cart.tries() > 0) {
        std::printf("S3 PRESSMARK  FINISHED MARK  impression on the gold line  pulls %d  (%.1f s)\n", cart.pulls(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PRESSMARK  OPEN  no finished mark  pulls %d  (%.1f s)\n", cart.pulls(), frames / 60.0);
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
            std::printf("S3 PRESSMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pressmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pressmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
