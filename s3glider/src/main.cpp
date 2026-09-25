// S3 GLIDER
//   s3glider                 play
//   s3glider --sim           autopilot rides the ridge and lands
//   s3glider --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/glider.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        glider::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    glider::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool air = false, field = false;
    int frames = 0;
    const int limit = 60 * 220;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!air && cart.flightX() > 420.f) {
            save(sys, "ridge.png");
            air = true;
        }
        if (!field && cart.flightX() > 2300.f) {
            save(sys, "field.png");
            field = true;
        }
    }
    save(sys, "end.png");
    const char* word = cart.won() ? "PASS" : "FAIL";
    std::printf("S3 GLIDER  %s  %s  score %d  (%.1f s)\n", word, cart.result(), cart.score(), frames / 60.0);
    if (!cart.won()) {
        std::fprintf(stderr, "glider x %.0f agl %.1f spd %.1f var %.2f\n", cart.flightX(), cart.flightAgl(),
                     cart.flightSpd(), cart.flightVar());
    }
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GLIDER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3glider [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<glider::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
