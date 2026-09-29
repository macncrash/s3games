// S3 BOARDMARK
//   s3boardmark                 patch the mark, then leave
//   s3boardmark --sim           autopilot finishes the mark and leaves
//   s3boardmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/boardmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    boardmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool desk = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!desk && frames == 18) {
            save(sys, "desk.png");
            desk = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.left()) {
        std::printf("S3 BOARDMARK  FINISHED MARK  the mark held  left the desk  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BOARDMARK  OPEN  the mark did not finish  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 BOARDMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3boardmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<boardmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
