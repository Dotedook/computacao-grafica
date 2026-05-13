#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

constexpr int kWindowWidth  = 960;
constexpr int kWindowHeight = 540;

const glm::vec3 kPalette[] = {
    {1.0f,  0.35f, 0.35f},
    {0.35f, 0.70f, 1.0f},
    {0.35f, 0.85f, 0.45f},
    {1.0f,  0.85f, 0.25f},
    {0.80f, 0.40f, 1.0f},
    {0.25f, 0.85f, 0.80f},
};
constexpr int kPaletteSize = 6;

struct SceneObject {
    std::string name;
    GLuint      vao = 0, vbo = 0;
    GLuint      textureId = 0;
    int         vertexCount = 0;
    glm::vec3   position{0.0f};
    glm::vec3   rotation{0.0f};
    glm::vec3   scale{1.0f};
    glm::vec3   color{0.8f};
};

std::vector<SceneObject> gObjects;
int gSelected = 0;

// ---- OBJ loader ----

struct FaceIdx { int v = 0, t = -1, n = -1; };

FaceIdx ParseFaceToken(const std::string& tok)
{
    FaceIdx fi;
    std::istringstream ss(tok);
    std::string index;
    if (std::getline(ss, index, '/')) {
        if (!index.empty()) fi.v = std::stoi(index) - 1;
    }
    if (std::getline(ss, index, '/')) {
        if (!index.empty()) fi.t = std::stoi(index) - 1;
    }
    if (std::getline(ss, index)) {
        if (!index.empty()) fi.n = std::stoi(index) - 1;
    }
    return fi;
}

// Buffer layout: pos(3) + texcoord(2) + normal(3) = 8 floats/vertex
std::vector<float> LoadOBJ(const std::string& path, int& vertexCount,
                            std::string& name, std::string& mtlFile)
{
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open: " << path << '\n';
        vertexCount = 0;
        return {};
    }

    size_t slash = path.find_last_of("/\\");
    size_t dot   = path.find_last_of('.');
    name = (slash != std::string::npos && dot > slash)
               ? path.substr(slash + 1, dot - slash - 1)
               : path;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<float>     out;
    mtlFile.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;

        if (tok == "mtllib") {
            ss >> mtlFile;
        } else if (tok == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            positions.push_back({x, y, z});
        } else if (tok == "vt") {
            float s, t;
            ss >> s >> t;
            texCoords.push_back({s, t});
        } else if (tok == "vn") {
            float x, y, z;
            ss >> x >> y >> z;
            normals.push_back({x, y, z});
        } else if (tok == "f") {
            std::vector<FaceIdx> fv;
            std::string s;
            while (ss >> s) fv.push_back(ParseFaceToken(s));
            if (fv.size() < 3) continue;
            auto addVert = [&](const FaceIdx& fi) {
                if (fi.v < 0 || fi.v >= (int)positions.size()) return;
                glm::vec3 pos = positions[fi.v];
                glm::vec2 tc  = (fi.t >= 0 && fi.t < (int)texCoords.size())
                                    ? texCoords[fi.t] : glm::vec2(0.0f, 0.0f);
                glm::vec3 nm  = (fi.n >= 0 && fi.n < (int)normals.size())
                                    ? normals[fi.n] : glm::vec3(0.0f, 0.0f, 1.0f);
                out.push_back(pos.x); out.push_back(pos.y); out.push_back(pos.z);
                out.push_back(tc.s);  out.push_back(tc.t);
                out.push_back(nm.x);  out.push_back(nm.y);  out.push_back(nm.z);
            };
            // Fan triangulation — handles quads and n-gons
            for (int i = 1; i + 1 < (int)fv.size(); i++) {
                addVert(fv[0]);
                addVert(fv[i]);
                addVert(fv[i + 1]);
            }
        }
    }

    vertexCount = (int)out.size() / 8;
    return out;
}

// Returns texture filename from map_Kd in the MTL file
std::string ParseMTL(const std::string& mtlPath)
{
    std::ifstream file(mtlPath);
    if (!file) return {};
    std::string line, texFile;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;
        if (tok == "map_Kd") {
            ss >> texFile;
            break;
        }
    }
    return texFile;
}

GLuint LoadTexture(const std::string& path)
{
    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << '\n';
        return 0;
    }
    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return id;
}

bool UploadObject(const std::string& path, int paletteIdx)
{
    int         vertCount = 0;
    std::string name, mtlFile;
    auto        verts = LoadOBJ(path, vertCount, name, mtlFile);
    if (vertCount == 0) return false;

    SceneObject obj;
    obj.name        = name;
    obj.vertexCount = vertCount;
    obj.color       = kPalette[paletteIdx % kPaletteSize];

    if (!mtlFile.empty()) {
        std::string objDir;
        size_t slash = path.find_last_of("/\\");
        if (slash != std::string::npos) objDir = path.substr(0, slash + 1);
        std::string texFile = ParseMTL(objDir + mtlFile);
        if (!texFile.empty())
            obj.textureId = LoadTexture(objDir + texFile);
    }

    glGenVertexArrays(1, &obj.vao);
    glGenBuffers(1, &obj.vbo);
    glBindVertexArray(obj.vao);
    glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    gObjects.push_back(std::move(obj));
    return true;
}

// ---- Shaders ----

GLuint CompileShader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        std::cerr << "Shader error: " << log << '\n';
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint CreateProgram(const char* vert, const char* frag)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vert);
    if (!vs) return 0;
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, frag);
    if (!fs) { glDeleteShader(vs); return 0; }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Link error: " << log << '\n';
        glDeleteProgram(prog);
        prog = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---- Input ----

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_TAB:
        if (!gObjects.empty())
            gSelected = (gSelected + 1) % (int)gObjects.size();
        break;
    }
}

void ProcessContinuousInput(GLFWwindow* window, float dt)
{
    if (gObjects.empty()) return;
    auto& obj = gObjects[gSelected];

    const float rotSpeed   = 1.5f * dt;
    const float moveSpeed  = 2.0f * dt;
    const float scaleSpeed = 0.8f * dt;

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) obj.rotation.x += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) obj.rotation.y += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) obj.rotation.z += rotSpeed;

    if (glfwGetKey(window, GLFW_KEY_D)    == GLFW_PRESS) obj.position.x += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A)    == GLFW_PRESS) obj.position.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_W)    == GLFW_PRESS) obj.position.z -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S)    == GLFW_PRESS) obj.position.z += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP)   == GLFW_PRESS) obj.position.y += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) obj.position.y -= moveSpeed;

    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
        obj.scale += glm::vec3(scaleSpeed);
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
        obj.scale = glm::max(obj.scale - glm::vec3(scaleSpeed), glm::vec3(0.05f));

    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    bool ctrl  = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)  == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    if (shift) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) obj.scale.x += scaleSpeed;
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) obj.scale.y += scaleSpeed;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) obj.scale.z += scaleSpeed;
    }
    if (ctrl) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            obj.scale.x = std::max(0.05f, obj.scale.x - scaleSpeed);
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            obj.scale.y = std::max(0.05f, obj.scale.y - scaleSpeed);
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            obj.scale.z = std::max(0.05f, obj.scale.z - scaleSpeed);
    }
}

} // namespace

// =============================================================================

int main(int argc, char** argv)
{
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight,
                                          "OBJ3D - Eduardo Kuhn", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window create failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, KeyCallback);

    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        layout(location = 2) in vec3 aNormal;
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        out vec2 vTexCoord;
        out vec3 vNormal;
        void main() {
            gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
            vTexCoord = aTexCoord;
            vNormal = mat3(transpose(inverse(uModel))) * aNormal;
        }
    )";
    const char* fragSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        in vec3 vNormal;
        uniform sampler2D uTexture;
        uniform vec3 uColor;
        uniform bool uHasTexture;
        out vec4 FragColor;
        void main() {
            if (uHasTexture) {
                FragColor = texture(uTexture, vTexCoord);
            } else {
                FragColor = vec4(uColor, 1.0);
            }
        }
    )";

    GLuint program = CreateProgram(vertSrc, fragSrc);
    if (!program) { glfwDestroyWindow(window); glfwTerminate(); return 1; }

    GLint locModel      = glGetUniformLocation(program, "uModel");
    GLint locView       = glGetUniformLocation(program, "uView");
    GLint locProjection = glGetUniformLocation(program, "uProjection");
    GLint locColor      = glGetUniformLocation(program, "uColor");
    GLint locHasTex     = glGetUniformLocation(program, "uHasTexture");
    GLint locTexture    = glGetUniformLocation(program, "uTexture");

    glEnable(GL_DEPTH_TEST);

    std::vector<std::string> objPaths;
    for (int i = 1; i < argc; i++)
        objPaths.push_back(argv[i]);

    if (objPaths.empty()) {
        for (const char* dir : {"./assets", "../assets"}) {
            std::filesystem::path dp(dir);
            if (std::filesystem::exists(dp)) {
                for (const auto& entry : std::filesystem::directory_iterator(dp)) {
                    if (entry.path().extension() == ".obj")
                        objPaths.push_back(entry.path().string());
                }
                if (!objPaths.empty()) break;
            }
        }
    }

    if (objPaths.empty()) {
        std::cerr << "Usage: Hello3D <file1.obj> [file2.obj ...]\n";
        std::cerr << "  or place .obj files in ./assets/\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::sort(objPaths.begin(), objPaths.end());

    for (int i = 0; i < (int)objPaths.size(); i++)
        UploadObject(objPaths[i], i);

    if (gObjects.empty()) {
        std::cerr << "No objects loaded successfully\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    float spacing   = 2.5f;
    float totalSpan = spacing * ((int)gObjects.size() - 1);
    for (int i = 0; i < (int)gObjects.size(); i++)
        gObjects[i].position.x = -totalSpan * 0.5f + i * spacing;

    const glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 1.5f, 7.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt  = now - lastTime;
        lastTime  = now;

        glfwPollEvents();
        ProcessContinuousInput(window, dt);

        {
            const auto& sel = gObjects[gSelected];
            std::string title =
                "OBJ3D | [" + sel.name + "] " +
                std::to_string(gSelected + 1) + "/" + std::to_string(gObjects.size()) +
                " | TAB=select  X/Y/Z=rot  WASD=move  Up/Dn=Y  [/]=scale  Shift+XYZ=scale+  Ctrl+XYZ=scale-";
            glfwSetWindowTitle(window, title.c_str());
        }

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        glClearColor(0.08f, 0.11f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float     aspect = fbH > 0 ? (float)fbW / (float)fbH : 1.0f;
        glm::mat4 proj   = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        glUseProgram(program);
        glUniformMatrix4fv(locView,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(locTexture, 0);

        for (int i = 0; i < (int)gObjects.size(); i++) {
            const auto& obj = gObjects[i];

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, obj.rotation.x, {1.0f, 0.0f, 0.0f});
            model = glm::rotate(model, obj.rotation.y, {0.0f, 1.0f, 0.0f});
            model = glm::rotate(model, obj.rotation.z, {0.0f, 0.0f, 1.0f});
            model = glm::scale(model, obj.scale);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(obj.vao);

            bool hasTex = (obj.textureId != 0);
            if (hasTex) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, obj.textureId);
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUniform1i(locHasTex, hasTex ? GL_TRUE : GL_FALSE);
            float     dim = (i == gSelected) ? 1.0f : 0.50f;
            glm::vec3 c   = obj.color * dim;
            glUniform3fv(locColor, 1, glm::value_ptr(c));
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);

            // Yellow wireframe overlay on selected object
            if (i == gSelected) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glUniform1i(locHasTex, GL_FALSE);
                glm::vec3 wc{1.0f, 0.92f, 0.25f};
                glUniform3fv(locColor, 1, glm::value_ptr(wc));
                glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
                glDisable(GL_POLYGON_OFFSET_LINE);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            if (hasTex) glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    for (auto& obj : gObjects) {
        glDeleteBuffers(1, &obj.vbo);
        glDeleteVertexArrays(1, &obj.vao);
        if (obj.textureId) glDeleteTextures(1, &obj.textureId);
    }
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
