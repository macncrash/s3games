// S3 CAUSEWAY WELL
//   s3causewaywell                 keep the well standing
//   s3causewaywell --sim           autopilot holds three waves
//   s3causewaywell --sim --shots D also writes PNGs into D
//
//   Left and right change lane. Z or C fires. Enter starts.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/well.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        cwell::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cwell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false, end = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (shotDir && !fight && m == 1 && frames == 80) {
            save(sys, "causeway.png");
            fight = true;
        } else if (shotDir && !end && m == 3) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 CAUSEWAY WELL  THE WELL STANDS  score %d\n", cart.score());
        return 0;
    }
    std::printf("S3 CAUSEWAY WELL  THE WELL FALLS  score %d  wave %d  well %d\n", cart.score(), cart.wave() + 1,
                cart.well());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CAUSEWAY WELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3causewaywell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cwell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
