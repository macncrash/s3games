// S3 LUGE BOOM
//   s3lugeboom                 play
//   s3lugeboom --sim           autopilot delivers the drive
//   s3lugeboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/luge.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        luge::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    luge::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool mid = false;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.traveled() >= 280.f) {
            save(sys, "chute.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("%s\n", cart.result());
        std::fflush(stdout);
        return 0;
    }
    if (cart.result()[0])
        std::printf("%s\n", cart.result());
    else
        std::printf("S3 LUGE BOOM  FAIL  unfinished  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 LUGE BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lugeboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<luge::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
