#include <v4r.hpp>
#include <v4r/debug.hpp>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <chrono>

using namespace std;
using namespace v4r;

constexpr uint32_t num_frames = 10000;

shared_ptr<Scene> loadScene(AssetLoader &loader, const char *mesh_path,
                            const char *texture_path)
{
    vector<shared_ptr<Mesh>> meshes;
    vector<shared_ptr<Material>> materials;

    meshes.push_back(loader.loadMesh(mesh_path));

    auto texture = loader.loadTexture(texture_path);
    materials.push_back(
            loader.makeMaterial(LitRendererInputs::MaterialDescription {
        move(texture)
    }));

    SceneDescription scene_desc(move(meshes), move(materials));
    scene_desc.addInstance(0, 0, glm::translate(glm::mat4(1.f),
                                                glm::vec3(0.f, 0.f, -2.f)));
    scene_desc.addLight(glm::vec3(0.f, 2.f, 1.f),
                        glm::vec3(1.f, 0.f, 0.f));

    scene_desc.addLight(glm::vec3(0.5f, 2.f, -1.9f),
                        glm::vec3(0.f, 1.f, 0.f));

    return loader.makeScene(scene_desc);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << argv[0] << " geometry_path texture_path batch_size | save" <<
            endl;
        exit(EXIT_FAILURE);
    }

    bool bench;
    uint32_t batch_size;
    if (!strcmp(argv[3], "save")) {
        bench = false;
        batch_size = 1;
    }  else {
        bench = true;
        batch_size = stoul(argv[3]);
    }

    BatchRenderer renderer({0, 1, 1, batch_size, 256, 256,
        glm::mat4(1.f),
        {
            RenderFeatures::MeshColor::Texture,
            RenderFeatures::Pipeline::Lit,
            RenderFeatures::Outputs::Color ,
            RenderFeatures::Options::CpuSynchronization |
                RenderFeatures::Options::DoubleBuffered
        }
    });

    auto loader = renderer.makeLoader();
    auto scene = loadScene(loader, argv[1], argv[2]);

    CommandStream cmd_stream = renderer.makeCommandStream();
    vector<Environment> envs;

    for (uint32_t batch_idx = 0; batch_idx < batch_size; batch_idx++) {
        envs.emplace_back(cmd_stream.makeEnvironment(scene, 90));

        envs[batch_idx].setCameraView(glm::mat4(1.f));
    }

    if (bench) {
        auto start = chrono::steady_clock::now();

        uint32_t num_iters = num_frames / batch_size;

        RenderSync prevsync = cmd_stream.render(envs);

        for (uint32_t i = 1; i < num_iters; i++) {
            RenderSync newsync = cmd_stream.render(envs);
            prevsync.cpuWait();
            prevsync = move(newsync);
        }

        auto end = chrono::steady_clock::now();

        auto diff = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "FPS: " << ((double)num_iters * (double)batch_size /
                (double)diff.count()) * 1000.0 << endl;
    } else {
        RenderDoc rdoc {};
        rdoc.startFrame();

        auto sync = cmd_stream.render(envs);
        sync.cpuWait();
        saveFrame("/tmp/out_color.bmp", cmd_stream.getColorDevPtr(),
                  256, 256, 4);

        rdoc.endFrame();
    }
}
