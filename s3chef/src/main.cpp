// S3 CHEF
//   s3chef                 play
//   s3chef --sim           autopilot plates the ten tickets
//   s3chef --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/chef.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        chef::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    chef::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, late = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.plated() >= 1) {
            save(sys, "pass.png");
            mid = true;
        } else if (shotDir && !late && cart.plated() >= 5) {
            save(sys, "rush.png");
            late = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 CHEF  PASS  plated 10 of 10 before they burned  score %d  (%.1fs)\n", cart.score(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.fail() : "timed out";
    std::printf("S3 CHEF  FAIL  plated %d of 10  score %d  %s  (%.1fs)\n", cart.plated(), cart.score(), why, frames / 60.0);
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
            std::printf("S3 CHEF %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3chef [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<chef::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
