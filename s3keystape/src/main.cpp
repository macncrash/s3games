// S3 KEYSTAPE
//   s3keystape                 play
//   s3keystape --sim           file the keys until the drawer matches, then leave
//   s3keystape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/keys.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System titleSys(true);
        keys::Game title;
        titleSys.bootCart(title);
        for (int i = 0; i < 8; i++) titleSys.step();
        save(titleSys, "title.png");
    }

    gs::System sys(true);
    keys::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    for (int i = 0; i < 6; i++) sys.step();
    save(sys, "desk.png");

    int frames = 6;
    const int limit = 60 * 20;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames > 40) {
            save(sys, "keys.png");
            mid = true;
        }
    }
    save(sys, "leave.png");
    if (!cart.won()) {
        std::printf("S3 KEYSTAPE  FAIL\n");
        return 1;
    }
    std::string cuts;
    for (int k = 0; k < keys::NKEYS; k++) {
        if (k) cuts += ' ';
        for (int w = 0; w < keys::NWARDS; w++) {
            if (w) cuts += '-';
            cuts += char('0' + cart.tapeCut(k, w));
        }
    }
    std::printf("the drawer matches the tape (%s) and you leave\n", cuts.c_str());
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 KEYSTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3keystape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<keys::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
