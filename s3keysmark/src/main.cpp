// S3 KEYSMARK
//   s3keysmark                 play the marked phrase
//   s3keysmark --sim           autopilot finishes the mark
//   s3keysmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/keysmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        keysmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    keysmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.notes() >= 3) {
            save(sys, "play.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished()) {
        std::printf("S3 KEYSMARK  FINISHED MARK  phrase held  notes %d  slips %d  (%.1f s)\n", cart.notes(),
                    cart.slips(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 KEYSMARK  OPEN  no finished mark  notes %d  slips %d  (%.1f s)\n", cart.notes(), cart.slips(),
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
            std::printf("S3 KEYSMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3keysmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<keysmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
