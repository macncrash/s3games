// S3 CRANE BOOM
//   s3craneboom                 deliver the drive to the boom
//   s3craneboom --sim           autopilot sets the drive on the end
//   s3craneboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/game.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        craneboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    craneboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    save(sys, "end.png");
    if (cart.won()) std::printf("S3 CRANE BOOM  WIN  drive set on the end of the boom\n");
    else if (!cart.over()) std::printf("S3 CRANE BOOM  FAIL  the leg ran out\n");
    else std::printf("S3 CRANE BOOM  FAIL  %s\n", cart.reason());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CRANE BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3craneboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<craneboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
