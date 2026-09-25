// S3 ROW
//   s3row                 row five hundred meters
//   s3row --sim           the shell stays in the lane
//   s3row --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/row.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    row::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);

    bool title = false, race = false, end = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 12) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && !race && cart.meters() > 180) {
            save(sys, "race.png");
            race = true;
        } else if ((m == 2 || m == 3) && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.over()) {
        std::printf("S3 ROW  FAIL  did not finish  %d m\n", cart.meters());
        return 1;
    }
    std::printf("%s\n", cart.report());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 ROW %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3row [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<row::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
