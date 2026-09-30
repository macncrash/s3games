// S3 CULVERT DOOR
//   s3culvertdoor                 hold the door
//   s3culvertdoor --sim           autopilot keeps the leaf shut
//   s3culvertdoor --sim --shots D also writes PNGs into D
//
//   Hold Z, X, C, or Space to brace. Left and Right meet the surge.
//   Enter starts. Miss the surge and the door walks, or the water takes the lock.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/door.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    culvertdoor::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (shotDir) {
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }
    int frames = 0;
    const int limit = 60 * 200;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.secondsLeft() < 150 && cart.secondsLeft() > 140) {
            save(sys, "door.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "held.png" : "end.png");
    if (cart.won()) {
        std::printf("S3 CULVERT DOOR  HELD THE DOOR THREE MINUTES  the culvert is done  (%.1f s)\n", frames / 60.0);
    } else {
        std::printf("S3 CULVERT DOOR  FAIL  watch %ds left  seal %d%%  water %d%%  (%.1f s)\n", cart.secondsLeft(),
                    cart.sealPct(), cart.waterPct(), frames / 60.0);
    }
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CULVERT DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3culvertdoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<culvertdoor::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
