// PALISADE
//   s3palisadedoor                 play
//   s3palisadedoor --sim           autopilot holds the door
//   s3palisadedoor --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/door.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        palisade::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    palisade::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 180 * 60 + 180;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.held() > 90) {
            save(sys, "gate.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("WIN  the palisade held the door for %d seconds  door %d\n", cart.held() / 60, cart.door());
        return 0;
    }
    std::printf("FAIL  the palisade lost the door  held %d  door %d\n", cart.held() / 60, cart.door());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("PALISADE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3palisadedoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<palisade::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
