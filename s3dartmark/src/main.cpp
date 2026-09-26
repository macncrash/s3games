// S3 DARTMARK
//   s3dartmark                 one visit, finish the mark on 20
//   s3dartmark --sim           autopilot closes 20
//   s3dartmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/dartmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    dartmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool twenty = std::strstr(cart.bed(), "20") != nullptr;
    if (cart.rules() && cart.won() && cart.finished() && cart.marks() == 3 && twenty) {
        std::printf("S3 DARTMARK  FINISHED MARK  %s  closed 20  darts %d  (%.1f s)\n", cart.bed(), cart.darts(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DARTMARK  OPEN  no finished mark  %s  marks %d  darts %d  (%.1f s)\n", cart.bed(), cart.marks(),
                cart.darts(), frames / 60.0);
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
            std::printf("S3 DARTMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3dartmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<dartmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
