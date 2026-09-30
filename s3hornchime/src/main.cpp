// S3 HORNCHIME
//   s3hornchime                 a short horn; the hour has to chime
//   s3hornchime --sim           autopilot sounds the horn as the hour strikes
//   s3hornchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/hornchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    hornchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, horn = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!horn && cart.sounding()) {
            save(sys, "horn.png");
            horn = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.calls() >= 1 && cart.hour() == 12 && cart.minute() == 0 && cart.second() < 4 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 HORNCHIME  WIN  the hour chimes  %d:%02d:%02d  call %d  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), cart.calls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HORNCHIME  FAIL  %s  %d:%02d:%02d  call %d  (%.1f s)\n", cart.over() ? cart.reason() : "timed out",
                cart.hour(), cart.minute(), cart.second(), cart.calls(), frames / 60.0);
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
            std::printf("S3 HORNCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3hornchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hornchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
