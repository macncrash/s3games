// S3 MINE
//   s3mine                 ride the cart
//   s3mine --sim           autopilot shoots the props and misses the walls
//   s3mine --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mine.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    mine::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, riding = false;
    int frames = 0;
    const int limit = 60 * 60;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!riding && frames == 220) {
            save(sys, "ride.png");
            riding = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        if (cart.wrecks() == 0) {
            std::printf("S3 MINE  DAYLIGHT  shot %d props  walls clear  score %d  (%.1f s)\n", cart.shot(),
                        cart.score(), frames / 60.0);
        } else {
            std::printf("S3 MINE  DAYLIGHT  shot %d props  wrecks %d  score %d  (%.1f s)\n", cart.shot(),
                        cart.wrecks(), cart.score(), frames / 60.0);
        }
        return 0;
    }
    const char* why = cart.endNote()[0] ? cart.endNote() : "UNFINISHED";
    std::printf("S3 MINE  CAVE-IN  %s  shot %d/%d  lives %d  (%.1f s)\n", why, cart.shot(), cart.props(),
                cart.lives(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MINE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mine [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mine::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
