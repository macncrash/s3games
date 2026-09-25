// S3 SHELVE
//   s3shelve                 play
//   s3shelve --sim           autopilot empties the cart
//   s3shelve --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/shelve.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        shelve::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    shelve::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 60;
    bool play = false, back = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!play && m == 1 && cart.shelved() >= 1) {
            save(sys, "play.png");
            play = true;
        }
        if (!back && m == 3) {
            save(sys, "back.png");
            back = true;
        }
    }
    save(sys, "end.png");
    if (cart.won())
        std::printf("S3 SHELVE  WIN  the cart is clear  shelved %d  returned %d\n", cart.shelved(), cart.returned());
    else
        std::printf("S3 SHELVE  FAIL  the cart came back  shelved %d  returned %d\n", cart.shelved(), cart.returned());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SHELVE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shelve [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shelve::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
