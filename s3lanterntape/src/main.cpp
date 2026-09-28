// S3 LANTERNTAPE
//   s3lanterntape                 a short lantern
//   s3lanterntape --sim           autopilot files the drawer and shuts it
//   s3lanterntape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lanterntape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lanterntape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 LANTERNTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false, lane = false, drawer = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!lane && cart.walking()) {
            save(sys, "lane.png");
            lane = true;
        }
        if (!drawer && cart.phase() == 3) {
            save(sys, "drawer.png");
            drawer = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "WICK") == 0 && std::strcmp(cart.tapeLabel(1), "GLASS") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "HOOK") == 0;
    bool scores = cart.tapeScore(0) == 5 && cart.tapeScore(1) == 3 && cart.tapeScore(2) == 4;
    if (cart.won() && cart.shut() && cart.matched() && cart.rules() && names && scores && cart.sets() == 3 &&
        cart.drawerScore() == 12 && cart.traps() == 0 && cart.held(0) && cart.held(1) && cart.held(2)) {
        std::printf("the drawer matches the tape (WICK, GLASS, HOOK) and the short lantern is shut\n");
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LANTERNTAPE  FAIL  %s  sets %d  till %d  traps %d  %d%d%d  (%.1f s)\n", cart.reason(), cart.sets(),
                cart.drawerScore(), cart.traps(), cart.held(0) ? 1 : 0, cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0,
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
            std::printf("S3 LANTERNTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lanterntape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lanterntape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
