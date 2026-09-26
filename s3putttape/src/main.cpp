// S3 PUTTTAPE
//   s3putttape                 play putt until the drawer matches the tape
//   s3putttape --sim           autopilot putts, then leaves
//   s3putttape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/putttape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    putttape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false, drawer = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && cart.rolling()) {
            save(sys, "putt.png");
            rolling = true;
        }
        if (!drawer && cart.held(0) && cart.held(1) && cart.held(2)) {
            save(sys, "drawer.png");
            drawer = true;
        }
    }
    save(sys, "end.png");
    bool labels = std::strcmp(cart.tapeLabel(0), "QUARTER") == 0 && std::strcmp(cart.tapeLabel(1), "DIME") == 0 &&
                  std::strcmp(cart.tapeLabel(2), "NICKEL") == 0;
    if (cart.won() && cart.matched() && labels && cart.putts() >= 3 && cart.putts() <= 5 && cart.drawerCents() == 40) {
        std::printf("S3 PUTTTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PUTTTAPE  FAIL  %s  putts %d  till %d  q %d d %d n %d  (%.1f s)\n", cart.reason(), cart.putts(),
                cart.drawerCents(), cart.held(0) ? 1 : 0, cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0, frames / 60.0);
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
            std::printf("S3 PUTTTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3putttape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<putttape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
