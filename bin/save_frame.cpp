#include <v4r.hpp>
#include <v4r/debug.hpp>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace v4r;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Specify scene" << endl;
        exit(EXIT_FAILURE);
    }

    BatchRenderer renderer({0, 1, 1, 1, 256, 256,
        glm::mat4(
            1, 0, 0, 0,
            0, -1.19209e-07, -1, 0,
            0, 1, -1.19209e-07, 0,
            0, 0, 0, 1
        )
    });

    RenderDoc rdoc {};

    rdoc.startFrame();
    auto loader = renderer.makeLoader();
    auto scene = loader.loadScene(argv[1]);
    auto cmd_stream = renderer.makeCommandStream();
    cmd_stream.initState(0, scene, 90, 0.01, 1000);
    cmd_stream.setCameraView(0, glm::inverse(glm::mat4(
                  -1.19209e-07, 0, 1, 0,
                  0, 1, 0, 0,
                  -1, 0, -1.19209e-07, 0,
                  -3.38921, 1.62114, -3.34509, 1)));

    auto sync = cmd_stream.render();
    sync.cpuWait();
    rdoc.endFrame();

    saveFrame("/tmp/out_color.bmp", cmd_stream.getColorDevPtr(), 256, 256, 4);
    saveFrame("/tmp/out_depth.bmp", cmd_stream.getDepthDevPtr(), 256, 256, 1);

    loader.dropScene(move(scene));
}
