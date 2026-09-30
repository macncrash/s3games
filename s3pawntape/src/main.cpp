// S3 PAWNTAPE
//   s3pawntape                 play until the drawer matches the tape
//   s3pawntape --sim           autopilot pockets the tape and leaves
//   s3pawntape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pawn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pawntape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.left() && cart.matched() && cart.faults() == 0 && cart.filled() == 3 &&
        cart.drawerScore() == 15) {
        std::printf("S3 PAWNTAPE  PASS  the drawer matches the tape (%s %d, %s %d, %s %d) till %d (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeScore(0), cart.tapeLabel(1), cart.tapeScore(1), cart.tapeLabel(2),
                    cart.tapeScore(2), cart.drawerScore(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PAWNTAPE  FAIL  %s  till %d  filled %d  faults %d  (%.1f s)\n", cart.reason(), cart.drawerScore(),
                cart.filled(), cart.faults(), frames / 60.0);
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
            std::printf("S3 PAWNTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pawntape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pawntape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
