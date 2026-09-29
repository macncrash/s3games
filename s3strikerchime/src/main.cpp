// S3 STRIKER CHIME
//   s3strikerchime                 one swing, until the hour chimes
//   s3strikerchime --sim           autopilot, exits 0 only when the hour chimes
//   s3strikerchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/chime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        strikerchime::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    strikerchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    bool struck = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!struck && cart.chimed()) {
            save(sys, "chime.png");
            struck = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.chimed() && cart.hour() == 12 && cart.minute() == 0 && cart.second() == 0) {
        std::printf("S3 STRIKERCHIME  WIN  the hour chimes  %d:%02d:%02d  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3strikerchime %s\n", cart.reason());
    std::printf("S3 STRIKERCHIME  FAIL  hour silent  %d:%02d:%02d  (%.1f s)\n", cart.hour(), cart.minute(),
                cart.second(), frames / 60.0);
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
            std::printf("S3 STRIKER CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3strikerchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<strikerchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
