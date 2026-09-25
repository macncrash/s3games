// S3 WICKET
//   s3wicket                 play the over
//   s3wicket --sim           autopilot takes the wicket and clears the rope
//   s3wicket --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wicket.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    wicket::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 45;
    bool filed = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!filed && frames == 90) {
            save(sys, "over.png");
            filed = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        if (cart.wickets() > 0 && cart.sixes() > 0)
            std::printf("S3 WICKET  HIT THE WICKET AND CLEARS THE ROPE  wickets %d  sixes %d\n", cart.wickets(),
                        cart.sixes());
        else if (cart.wickets() > 0)
            std::printf("S3 WICKET  HIT THE WICKET  wickets %d  sixes %d\n", cart.wickets(), cart.sixes());
        else
            std::printf("S3 WICKET  CLEARS THE ROPE  wickets %d  sixes %d\n", cart.wickets(), cart.sixes());
        return 0;
    }
    std::printf("S3 WICKET  SIX BALLS GONE  wickets %d  sixes %d\n", cart.wickets(), cart.sixes());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 WICKET %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wicket [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wicket::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
