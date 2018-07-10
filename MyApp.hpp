#ifndef __amorphous_MyApp__
#define __amorphous_MyApp__

#include "Wasm.hpp"
#include <chrono>
#include <vector>
#include <array>
#include "wavefront_obj_reader.hpp"


class MyApp : public WasmApp {

    std::chrono::time_point<std::chrono::system_clock> start_time;

    int num_indices;

    int CalculateVertexNormals(
        const std::vector<vec3>& vertices,
        const std::vector< std::vector<int> >& indices,
        std::vector<vec3>& normals);

    int LoadModelFromObjFile(
        const std::string& obj_file_pathname,
        std::vector<float>& vertices,
        std::vector<float>& normals,
        std::vector<GLuint>& triangle_indices );


public:

    MyApp() : num_indices(0) {}

    int Init();

    void Render();
};

#endif /* __amorphous_MyApp__ */
