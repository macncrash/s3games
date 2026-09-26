// S3 DRAWER
//   s3drawer                 play
//   s3drawer --sim           file the till until it matches, then shut it
//   s3drawer --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/drawer.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System titleSys(true);
        drawer::Game title;
        titleSys.bootCart(title);
        for (int i = 0; i < 8; i++) titleSys.step();
        save(titleSys, "title.png");
    }

    gs::System sys(true);
    drawer::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    for (int i = 0; i < 8; i++) sys.step();
    save(sys, "till.png");

    int frames = 8;
    const int limit = 60 * 30;
    bool shut = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!shut && cart.night() == 2 && frames > 90) {
            save(sys, "count.png");
            shut = true;
        }
    }
    save(sys, "closed.png");
    if (!cart.won()) {
        std::printf("S3 DRAWER  FAIL  night %d\n", cart.night() + 1);
        return 1;
    }
    auto money = [](int c) {
        char b[16];
        std::snprintf(b, sizeof b, "$%d.%02d", c / 100, c % 100);
        return std::string(b);
    };
    std::printf("the drawer matches the tape (%s, %s, %s) and the shop is closed\n", money(cart.tape(0)).c_str(),
                money(cart.tape(1)).c_str(), money(cart.tape(2)).c_str());
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DRAWER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3drawer [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<drawer::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
