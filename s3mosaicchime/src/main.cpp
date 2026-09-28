// S3 MOSAIC CHIME
//   s3mosaicchime            play the mosaic until the hour chimes, then leave
//   s3mosaicchime --sim      autopilot, exits 0 only when the hour chimes
//   s3mosaicchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mosaic.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        mosaicchime::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mosaicchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool laid = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!laid && frames == 40) {
            save(sys, "lay.png");
            laid = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < mosaicchime::kGraceSec;
    if (cart.won() && hour && cart.tiles() >= 1 && cart.tiles() <= mosaicchime::CELLS &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 MOSAICCHIME  WIN  the hour chimes  %d:%02d:%02d  tile %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.tiles(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MOSAICCHIME  OPEN  %d:%02d:%02d  tile %d  %s  (%.1f s)\n", cart.hour(), cart.minute(),
                cart.second(), cart.tiles(), cart.reason(), frames / 60.0);
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
            std::printf("S3 MOSAIC CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mosaicchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mosaicchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
