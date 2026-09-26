// S3 FAIR GOLD
//   s3fairgold                 play the midway until the gold double pays
//   s3fairgold --sim           autopilot leaves only when that is true
//   s3fairgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fairgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    fairgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, flying = false, walking = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (!flying && std::strcmp(cart.phase(), "flight") == 0) {
            save(sys, "toss.png");
            flying = true;
        }
        if (!walking && std::strcmp(cart.phase(), "walk") == 0) {
            save(sys, "gate.png");
            walking = true;
        }
    }
    save(sys, "end.png");
    const int bare = cart.gold() + cart.cream();
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.left() && math && cart.gold() >= 1 && cart.score() >= cart.fare() && bare < cart.fare() &&
        cart.rings() >= 1 && cart.rings() <= 3) {
        std::printf(
            "S3 FAIR GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  rings %d  (%.1f s)\n",
            cart.gold(), cart.cream(), cart.score(), cart.rings(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FAIR GOLD  STAY  gold %d  cream %d  score %d  rings %d  phase %s  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.score(), cart.rings(), cart.phase(), frames / 60.0);
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
            std::printf("S3 FAIR GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fairgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fairgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
