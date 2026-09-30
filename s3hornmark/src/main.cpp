// S3 HORNMARK
//   s3hornmark                 play the call
//   s3hornmark --sim           autopilot finishes the mark
//   s3hornmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/hornmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        hornmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    hornmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool blow = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (cart.blowing() && !blow) {
            save(sys, "call.png");
            blow = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.closed() && cart.notes() == 4 && !std::strcmp(cart.callName(), "closed")) {
        std::printf("S3 HORNMARK  FINISHED MARK  call closed  notes %d  (%.1f s)\n", cart.notes(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HORNMARK  OPEN  no finished mark  %s  notes %d  (%.1f s)\n", cart.callName(), cart.notes(),
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
            std::printf("S3 HORNMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3hornmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hornmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
