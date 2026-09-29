// S3 SOLITAIRE CHIME
//   s3solitairechime                 build the run and wait for the hour
//   s3solitairechime --sim           autopilot, exits 0 only when the hour chimes
//   s3solitairechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/solitaire.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    solitairechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 SOLITAIRECHIME  FAIL  rules  hour silent  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    int frames = 0;
    const int limit = 60 * 20;
    bool table = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!table && cart.built() > 0) {
            save(sys, "table.png");
            table = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.chimed() && cart.hour() == 12 && cart.minute() == 0 && cart.second() == 0 &&
        cart.built() == solitairechime::kCards && std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 SOLITAIRECHIME  WIN  the hour chimes  %d:%02d:%02d  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3solitairechime %s %s built %d %d:%02d:%02d\n", cart.phase(), cart.reason(), cart.built(),
                 cart.hour(), cart.minute(), cart.second());
    std::printf("S3 SOLITAIRECHIME  FAIL  hour silent  %d:%02d:%02d  built %d  (%.1f s)\n", cart.hour(), cart.minute(),
                cart.second(), cart.built(), frames / 60.0);
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
            std::printf("S3 SOLITAIRE CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3solitairechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<solitairechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
