// S3 WICKETMARK
//   s3wicketmark                 set the mark, bowl the wicket, lift the coin
//   s3wicketmark --sim           autopilot finishes the mark
//   s3wicketmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wicketmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        wicketmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    wicketmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int seen = 0;
    bool bowl = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (cart.bowling() && ++seen == 20 && !bowl) {
            save(sys, "bowl.png");
            bowl = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.lifted() && cart.opened() && cart.onMark() && cart.balls() > 0 &&
        !std::strcmp(cart.callName(), "wicket")) {
        std::printf("S3 WICKETMARK  FINISHED MARK  wicket  on the mark  coin lifted  balls %d  (%.1f s)\n",
                    cart.balls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 WICKETMARK  OPEN  no finished mark  %s  balls %d  (%.1f s)\n", cart.callName(), cart.balls(),
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
            std::printf("S3 WICKETMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wicketmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wicketmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
