// S3 GLIDER LANE
//   s3gliderlane                 stay in the lane for the leg
//   s3gliderlane --sim           autopilot keeps the lane and beats the crew
//   s3gliderlane --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lane.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        glane::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    glane::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool glide = false, mid = false, gate = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!glide && m >= 1 && frames > 24) {
            save(sys, "glide.png");
            glide = true;
        } else if (!mid && m == 2) {
            save(sys, "lane.png");
            mid = true;
        } else if (!gate && m == 3) {
            save(sys, "gate.png");
            gate = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 GLIDER LANE  FAIL  %s  x %.1f  alt %.1f  spd %.1f  vs %.2f  margin %.2f  crew %.1f  (%.1f s)\n",
            why, cart.x(), cart.alt(), cart.speed(), cart.vs(), cart.margin(), cart.crewLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GLIDER LANE  PASS  stayed in the lane for the whole leg  (%.1f s)\n", cart.seconds());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GLIDER LANE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gliderlane [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<glane::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
