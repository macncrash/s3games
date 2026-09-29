// S3 SOLITAIRE SEVEN
//   s3solitaireseven                 play until first to seven
//   s3solitaireseven --sim           autopilot builds the run and leaves
//   s3solitaireseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/solitaire.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    solitaire::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, dealt = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!titled && m == 0 && frames >= 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!dealt && m == 1) {
            save(sys, "table.png");
            dealt = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.you() >= 7 && cart.house() < 7) {
        std::printf("S3 SOLITAIRE SEVEN  PASS  first to seven  you %d  house %d  (%.1f s)\n", cart.you(), cart.house(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SOLITAIRE SEVEN  SHORT  you %d  house %d  (%.1f s)\n", cart.you(), cart.house(), frames / 60.0);
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
            std::printf("S3 SOLITAIRE SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3solitaireseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<solitaire::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
