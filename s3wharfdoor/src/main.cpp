// S3 WHARF DOOR
//   s3wharfdoor                 play
//   s3wharfdoor --sim           autopilot holds the freight door
//   s3wharfdoor --sim --shots D also writes PNGs into D
//
//   Hold Z, X, C, or Space to brace the door. Left and Right meet the tide.
//   Up pins the hoist when a crate drops. Three minutes. Miss it and the watch is over.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wharf.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    wharf::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (shotDir) {
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }
    int frames = 0;
    const int limit = 60 * 240;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.secondsLeft() < 150 && cart.secondsLeft() > 0) {
            save(sys, "wharf.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "held.png" : "end.png");
    int sec = cart.secondsLeft();
    if (cart.won()) {
        std::printf("S3 WHARF DOOR  WIN  the door held three minutes  gap %d%%  %d:%02d left  (%.1f s)\n",
                    cart.gapPct(), sec / 60, sec % 60, frames / 60.0);
        return 0;
    }
    std::printf("S3 WHARF DOOR  FAIL  the watch is over  gap %d%%  %d:%02d left  (%.1f s)\n", cart.gapPct(),
                sec / 60, sec % 60, frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 WHARF DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wharfdoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wharf::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
