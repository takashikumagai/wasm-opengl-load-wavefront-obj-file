#include "MyApp.hpp"
#include <cmath>
#include <GLES2/gl2.h>
#include <EGL/egl.h>


WasmApp *CreateWasmAppInstance() {
    return new MyApp;
}


static GLuint program = 0;

static const char *vertex_shader_source =
    "#version 300 es\n"\
    "layout(location=0) in vec4 pos;\n"\
    "layout(location=1) in vec3 n;\n"\
    "out vec3 normal;\n"\
    "uniform mat4 W;"\
    "uniform mat4 V;"\
    "uniform mat4 P;"\
    "uniform mat4 Rx;"\
    "uniform mat4 Ry;"\
    "uniform mat4 S;"\
    "void main() {gl_Position = P*V*W*Ry*Rx*S*pos;normal=mat3(W*Ry*Rx)*n;}";

static const char *fragment_shader_source =
    "#version 300 es\n"\
    "precision mediump float;\n"\
    "in vec3 normal;\n"\
    "layout(location=0) out vec4 fc;\n"\
    "uniform vec3 dir_to_light;\n"\
    "void main() {"\
        "vec3 n = normalize(normal);"\
        "float d = dot(dir_to_light,n);"\
        "float f = (d+1.0)*0.5;"\
        "vec3 c = f*vec3(1.0,1.0,1.0) + (1.0-f)*vec3(0.2,0.2,0.2);"\
        "fc = vec4(c.x,c.y,c.z,1.0);"\
    "}";


static GLuint vbo = 0;
static GLuint nbo = 0;
static GLuint ibo = 0;

static int CheckShaderStatus(GLuint shader) {

    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

    if(is_compiled == GL_TRUE) {
        return 0;
    } else {
        // Shader compilation failed.
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        // log_size == the long length INCLUDING the terminating null character
        std::vector<GLchar> error_log(log_length);
        glGetShaderInfoLog(shader, log_length, &log_length, &error_log[0]);

        printf("gl shader error: %s",&error_log[0]);

        glDeleteShader(shader);

        return -1;
    }
}

static int CheckProgramStatus(GLuint program) {

    GLint is_linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&is_linked);

    if(is_linked == GL_TRUE) {
        return 0;
    } else {
        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        std::vector<GLchar> error_log(log_length);
        glGetProgramInfoLog(program, log_length, &log_length, &error_log[0]);

        printf("gl program error: %s",&error_log[0]);

        glDeleteProgram(program);

        return -1;
    }
}

int MyApp::Init() {

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    if(vertex_shader == 0) {
        return -1;
    }

    GLint len = strlen(vertex_shader_source);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, &len);

    glCompileShader(vertex_shader);

    if( CheckShaderStatus(vertex_shader) < 0 ) {
        return -1;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    if(fragment_shader == 0) {
        return -1;
    }

    len = strlen(fragment_shader_source);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, &len);

    glCompileShader(fragment_shader);

    if( CheckShaderStatus(fragment_shader) < 0 ) {
        glDeleteShader(vertex_shader);
        return -1;
    }

    program = glCreateProgram();

    glAttachShader(program,vertex_shader);
    glAttachShader(program,fragment_shader);

    glLinkProgram(program);

    if( CheckProgramStatus(program) < 0 ) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return -1;
    }

    float ty = -3.0f;
    float m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        0.0f, ty,  15.0f, 1.0f  // column 4
    };

    float view[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        0.0f, 0.0f, 5.0f, 1.0f  // column 4
    };

    float proj[16] = {
        1.81f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 2.41f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.001f, 1.0f, // column 3
        0.0f, 0.0f, -0.1001f, 0.0f  // column 4
    };

    GLint world_transform      = glGetUniformLocation(program,"W");
    GLint view_transform       = glGetUniformLocation(program,"V");
    GLint projection_transform = glGetUniformLocation(program,"P");
    console_log(std::string("W: ") + std::to_string(world_transform));
    console_log(std::string("V: ") + std::to_string(view_transform));
    console_log(std::string("P: ") + std::to_string(projection_transform));
    glUseProgram(program);
    glUniformMatrix4fv(world_transform, 1, GL_FALSE, m);
    glUniformMatrix4fv(view_transform, 1, GL_FALSE, view);
    glUniformMatrix4fv(projection_transform, 1, GL_FALSE, proj);

    // Set the light direction vector
    float fv[] = {0.262705f, 0.938233f, 0.225176f};
    GLint dir_to_light = glGetUniformLocation(program,"dir_to_light");
    glUniform3fv(dir_to_light, 1, fv);

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<GLuint> indices;
    LoadModelFromObjFile("files/bunny.obj",vertices,normals,indices);

    this->num_indices = indices.size();

    // Set vertices - for each element we call 'Generate', 'Bind', and 'Buffer' APIs.
    glGenBuffers( 1, &vbo );
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers( 1, &nbo );
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * normals.size(), &normals[0], GL_STATIC_DRAW);

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    this->start_time = std::chrono::system_clock::now();

    return 0;
}

void MyApp::Render() {

    {
        static int count = 0;
        if(count < 5) {
            console_log("MyApp::Render");
            count += 1;
        }
    }
    //printf("");

    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> t = now - this->start_time;

	const float PI = 3.141593;
    float a = (float)fmod(t.count(),PI*2.0);
    float c = cosf(a);
    float s = sinf(a);
    float rx[16] = {
        //1.0f, 0.0f, 0.0f, 0.0f, // column 1
        //0.0f, c,    s,    0.0f, // column 2
        //0.0f, -s,   c,    0.0f, // column 3
        //0.0f, 0.0f, 0.0f, 1.0f  // column 4

        // Limit the rotation to around y-axis because two-axis rotation seems a bit random for this demo.
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        0.0f, 0.0f, 0.0f, 1.0f  // column 4
    };

    a = (float)fmod(t.count()*0.9,PI*2.0);
    c = cosf(a);
    s = sinf(a);
    float ry[16] = {
        c,    0.0f, -s,   0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s,    0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    GLint rotation_x = glGetUniformLocation(program,"Rx");
    GLint rotation_y = glGetUniformLocation(program,"Ry");
    glUniformMatrix4fv(rotation_x, 1, GL_FALSE, rx);
    glUniformMatrix4fv(rotation_y, 1, GL_FALSE, ry);

    float f = 40.0f; // scaling factor
    float scaling[16] = {
        f,    0.0f, 0.0f, 0.0f,
        0.0f, f,    0.0f, 0.0f,
        0.0f, 0.0f, f,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    GLint scale = glGetUniformLocation(program,"S");
    glUniformMatrix4fv(scale, 1, GL_FALSE, scaling);

    //unsigned int viewport_width = 1280;
    //unsigned int viewport_height = 720;
    //glViewport(0, 0, viewport_width, viewport_height);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    // Draws the cube model by calling OpenGL APIs
    // glBindBuffer(): bind a named buffer object.
    // glEnableVertexAttribArray(): enable a generic vertex attribute array.
    // glVertexAttribPointer(): define an array of generic vertex attribute data.

    GLuint vertex_attrib_index = 0;
    GLuint normal_attrib_index = 1;

    GLboolean normalized = GL_FALSE;

//  glBindBuffer( GL_ARRAY_BUFFER, 0 );
//  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );

    glEnableVertexAttribArray(vertex_attrib_index);
    glVertexAttribPointer(vertex_attrib_index, 3, GL_FLOAT, normalized, sizeof(float)*3, 0);

    // Drawing directly from a generic array - this does not work with emcc;
    //glVertexAttribPointer(vertex_attrib_index, 3, GL_FLOAT, normalized, stride, cube_vertices );

    glEnableVertexAttribArray(normal_attrib_index);
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glVertexAttribPointer(normal_attrib_index, 3, GL_FLOAT, normalized, sizeof(float)*3, 0);

    GLsizei num_elements_to_render = this->num_indices;
    //glDrawElements( GL_TRIANGLES, num_elements_to_render, GL_UNSIGNED_INT, cube_indices );
    glDrawElements( GL_TRIANGLES, num_elements_to_render, GL_UNSIGNED_INT, 0 );

    glDisableVertexAttribArray(vertex_attrib_index);
}

int MyApp::LoadModelFromObjFile(
    const std::string& obj_file_pathname,
    std::vector<float>& vertices,
    std::vector<float>& normals,
    std::vector<GLuint>& triangle_indices ) {

    std::vector<vec3> obj_vertices;
    std::vector<vec3> obj_normals;
    std::vector<vec2> obj_uvs;
    std::vector< std::vector<int> > obj_indices;

    read_wavefront_obj_from_file( obj_file_pathname, obj_vertices, obj_uvs, obj_normals, obj_indices );

    for(auto v:obj_vertices)
        vertices.insert(vertices.end(), v.begin(), v.end() );

    char buffer[512];
    sprintf(buffer,"vertex data (flattened to float array): %d", vertices.size());
    console_log(buffer);

    // We assume that the obj model is triangulated.
    for(auto triangle:obj_indices) {
        triangle_indices.push_back((GLuint)triangle[0]);
        triangle_indices.push_back((GLuint)triangle[1]);
        triangle_indices.push_back((GLuint)triangle[2]);
    }
        //triangle_indices.insert(triangle_indices.end(), triangle.begin(), triangle.end());

    sprintf(buffer,"index data (flattened to int array): %d", triangle_indices.size());
    console_log(buffer);

    if(obj_normals.size() == 0) {
        int ret = CalculateVertexNormals(obj_vertices, obj_indices, obj_normals);
        for(auto v:obj_normals)
            normals.insert(normals.end(), v.begin(), v.end() );

        return 0;
    }
    else {
        return 0;
    }
}

inline vec3 vec3_add(const vec3& lhs, const vec3& rhs) {
    vec3 v;
    v[0] = lhs[0] + rhs[0];
    v[1] = lhs[1] + rhs[1];
    v[2] = lhs[2] + rhs[2];
    return v;
}

inline vec3 vec3_sub(const vec3& lhs, const vec3& rhs) {
    vec3 v;
    v[0] = lhs[0] - rhs[0];
    v[1] = lhs[1] - rhs[1];
    v[2] = lhs[2] - rhs[2];
    return v;
}

inline vec3 cross_product(const vec3& lhs, const vec3& rhs) {
    return {
        lhs[1]*rhs[2] - lhs[2]*rhs[1],
        lhs[2]*rhs[0] - lhs[0]*rhs[2],
        lhs[0]*rhs[1] - lhs[1]*rhs[0]
    };
}

inline vec3 vec3_normalize(const vec3& v) {
    float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    return { v[0]/len, v[1]/len, v[2]/len };
}


/// \return list of faces which share the given vertex
static std::vector<int> find_faces_vertex_is_on(int vertex_index, const std::vector< std::vector<int> >& indices) {

    std::vector<int> face_indices;
    for(int i=0; i<indices.size(); i++) {

        // See if this i-th face has the given vertex
        auto itr = std::find( indices[i].begin(), indices[i].end(), vertex_index );
        if(itr != indices[i].end()) {
            face_indices.push_back(i); // Yes the vertex is in this i-th face
        }
    }

    return face_indices;
}

int MyApp::CalculateVertexNormals(
    const std::vector<vec3>& vertices,
    const std::vector< std::vector<int> >& indices,
    std::vector<vec3>& normals) {

    std::vector<vec3> face_normals;
    // Calculate normals for each face (polygon)
    for(auto f : indices) {
        vec3 v0 = vertices[f[0]];
        vec3 v1 = vertices[f[1]];
        vec3 v2 = vertices[f[2]];
        vec3 v01 = vec3_sub(v1,v0);
        vec3 v02 = vec3_sub(v2,v0);
        vec3 n = cross_product(v01,v02);

        face_normals.push_back(vec3_normalize(n));
    }

    normals.resize(vertices.size(), {0,0,0});
    const int num_verts = vertices.size();
    for(int i=0; i<num_verts; i++) {
        auto faces_vertex_is_on = find_faces_vertex_is_on(i,indices);
        vec3 n = {0,0,0};
        for(auto fi:faces_vertex_is_on)
            n = vec3_add(n,face_normals[fi]);
        normals[i] = vec3_normalize(n);
    }

    return 0;
}
