// S3 EIGHT GOLD
//   s3eightgold                 a short eight; only the gold counts double
//   s3eightgold --sim           autopilot, exits 0 only when the 8 clears in gold
//   s3eightgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/eightgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    eightgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, broke = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!broke && frames == 48) {
            save(sys, "break.png");
            broke = true;
        }
    }
    save(sys, "end.png");
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.eightGold() && cart.left() == 0 && cart.sunk() == 8 && cart.gold() >= 1 && math) {
        std::printf("S3 EIGHT GOLD  DOUBLE  only the gold counts double  8 IN %s  gold %d  cream %d  score %d  shots %d  (%.1f s)\n",
                    cart.pocketName(), cart.gold(), cart.cream(), cart.score(), cart.shots(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 EIGHT GOLD  SHORT  %s  gold %d  cream %d  score %d  left %d  sunk %d  shots %d  (%.1f s)\n",
                cart.reason(), cart.gold(), cart.cream(), cart.score(), cart.left(), cart.sunk(), cart.shots(),
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
            std::printf("S3 EIGHT GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3eightgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<eightgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
