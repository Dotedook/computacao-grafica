#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
  constexpr int kWindowWidth  = 960;
  constexpr int kWindowHeight = 540;

  struct CubeTransform
  {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // Euler angles in radians
    float     scale = 1.0f;
  };

  std::vector<CubeTransform> gCubes;
  int                        gSelectedCube = 0;

  // -------------------------------------------------------------------------

  GLuint CompileShader(GLenum type, const char *src)
  {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE)
    {
      GLint len = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
      std::string log(len, '\0');
      glGetShaderInfoLog(shader, len, nullptr, log.data());
      std::cerr << "Shader compile error: " << log << '\n';
      glDeleteShader(shader);
      return 0;
    }
    return shader;
  }

  GLuint CreateProgram(const char *vert, const char *frag)
  {
    const GLuint vs = CompileShader(GL_VERTEX_SHADER, vert);
    if (vs == 0) return 0;
    const GLuint fs = CompileShader(GL_FRAGMENT_SHADER, frag);
    if (fs == 0) { glDeleteShader(vs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE)
    {
      GLint len = 0;
      glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
      std::string log(len, '\0');
      glGetProgramInfoLog(prog, len, nullptr, log.data());
      std::cerr << "Program link error: " << log << '\n';
      glDeleteProgram(prog);
      prog = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
  }

  // -------------------------------------------------------------------------

  void KeyCallback(GLFWwindow *window, int key, int /*scan*/, int action, int /*mods*/)
  {
    if (action != GLFW_PRESS) return;

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;

    case GLFW_KEY_TAB:
      if (!gCubes.empty())
        gSelectedCube = (gSelectedCube + 1) % static_cast<int>(gCubes.size());
      break;

    case GLFW_KEY_N:
    {
      CubeTransform t;
      // Offset each new cube so they don't overlap
      t.position.x = static_cast<float>(gCubes.size()) * 1.8f - 0.9f;
      gCubes.push_back(t);
      gSelectedCube = static_cast<int>(gCubes.size()) - 1;
      break;
    }
    }
  }

  void ProcessContinuousInput(GLFWwindow *window, float dt)
  {
    if (gCubes.empty()) return;
    auto &t = gCubes[gSelectedCube];

    const float rotSpeed   = 1.8f * dt;
    const float moveSpeed  = 2.0f * dt;
    const float scaleSpeed = 0.8f * dt;

    // Rotacao: X Y Z
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) t.rotation.x += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) t.rotation.y += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) t.rotation.z += rotSpeed;

    // Translacao eixos X e Z: WASD
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) t.position.x += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) t.position.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) t.position.z -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) t.position.z += moveSpeed;

    // Translacao eixo Y: I J
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) t.position.y += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) t.position.y -= moveSpeed;

    // Escala uniforme: ] aumenta, [ diminui
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
      t.scale += scaleSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
      t.scale = std::max(0.05f, t.scale - scaleSpeed);
  }

  // -------------------------------------------------------------------------
  // Cubo: 6 faces * 2 triangulos * 3 vertices = 36 vertices
  // Cada vertice: x y z r g b  (6 floats)
  // Cores por face: frente=vermelho, tras=verde, esq=azul,
  //                 dir=amarelo, baixo=ciano, cima=magenta
  // clang-format off
  const float kCubeVertices[] = {
    // Frente (+Z) — vermelho
    -0.5f, -0.5f,  0.5f,  1.0f, 0.20f, 0.20f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.20f, 0.20f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.20f, 0.20f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.20f, 0.20f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.20f, 0.20f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.20f, 0.20f,

    // Tras (-Z) — verde
    -0.5f, -0.5f, -0.5f,  0.20f, 0.85f, 0.20f,
    -0.5f,  0.5f, -0.5f,  0.20f, 0.85f, 0.20f,
     0.5f,  0.5f, -0.5f,  0.20f, 0.85f, 0.20f,
     0.5f,  0.5f, -0.5f,  0.20f, 0.85f, 0.20f,
     0.5f, -0.5f, -0.5f,  0.20f, 0.85f, 0.20f,
    -0.5f, -0.5f, -0.5f,  0.20f, 0.85f, 0.20f,

    // Esquerda (-X) — azul
    -0.5f,  0.5f,  0.5f,  0.20f, 0.40f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.20f, 0.40f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.20f, 0.40f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.20f, 0.40f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.20f, 0.40f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.20f, 0.40f, 1.0f,

    // Direita (+X) — amarelo
     0.5f,  0.5f,  0.5f,  1.0f, 0.90f, 0.10f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.90f, 0.10f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.90f, 0.10f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.90f, 0.10f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.90f, 0.10f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.90f, 0.10f,

    // Baixo (-Y) — ciano
    -0.5f, -0.5f, -0.5f,  0.10f, 0.90f, 0.90f,
     0.5f, -0.5f, -0.5f,  0.10f, 0.90f, 0.90f,
     0.5f, -0.5f,  0.5f,  0.10f, 0.90f, 0.90f,
     0.5f, -0.5f,  0.5f,  0.10f, 0.90f, 0.90f,
    -0.5f, -0.5f,  0.5f,  0.10f, 0.90f, 0.90f,
    -0.5f, -0.5f, -0.5f,  0.10f, 0.90f, 0.90f,

    // Cima (+Y) — magenta
    -0.5f,  0.5f, -0.5f,  0.90f, 0.20f, 0.90f,
    -0.5f,  0.5f,  0.5f,  0.90f, 0.20f, 0.90f,
     0.5f,  0.5f,  0.5f,  0.90f, 0.20f, 0.90f,
     0.5f,  0.5f,  0.5f,  0.90f, 0.20f, 0.90f,
     0.5f,  0.5f, -0.5f,  0.90f, 0.20f, 0.90f,
    -0.5f,  0.5f, -0.5f,  0.90f, 0.20f, 0.90f,
  };
  // clang-format on
}

// =============================================================================

int main()
{
  if (!glfwInit())
  {
    std::cerr << "Erro ao inicializar GLFW.\n";
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(kWindowWidth, kWindowHeight,
                                        "Hello3D - Eduardo Kuhn", nullptr, nullptr);
  if (!window)
  {
    std::cerr << "Erro ao criar janela OpenGL.\n";
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, KeyCallback);

  // ---------------------------------------------------------------------------
  // Shaders

  const char *vertSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    out vec3 vColor;

    void main()
    {
      gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
      vColor = aColor;
    }
  )";

  const char *fragSrc = R"(
    #version 330 core
    in  vec3 vColor;
    out vec4 FragColor;
    void main() { FragColor = vec4(vColor, 1.0); }
  )";

  const GLuint program = CreateProgram(vertSrc, fragSrc);
  if (program == 0)
  {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  // ---------------------------------------------------------------------------
  // GPU buffers

  GLuint vao = 0, vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);

  constexpr GLsizei stride = 6 * sizeof(float);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // ---------------------------------------------------------------------------

  const GLint locModel      = glGetUniformLocation(program, "uModel");
  const GLint locView       = glGetUniformLocation(program, "uView");
  const GLint locProjection = glGetUniformLocation(program, "uProjection");

  glEnable(GL_DEPTH_TEST);

  // Cubo inicial centrado na origem
  gCubes.push_back(CubeTransform{});

  const glm::mat4 view = glm::lookAt(
      glm::vec3(0.0f, 1.2f, 4.5f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f));

  float lastTime = static_cast<float>(glfwGetTime());

  // ---------------------------------------------------------------------------
  // Loop principal

  while (!glfwWindowShouldClose(window))
  {
    const float now = static_cast<float>(glfwGetTime());
    const float dt  = now - lastTime;
    lastTime        = now;

    glfwPollEvents();
    ProcessContinuousInput(window, dt);

    // Titulo da janela com estado atual
    {
      const std::string title =
          "Hello3D  |  Cubo " + std::to_string(gSelectedCube + 1) +
          "/" + std::to_string(gCubes.size()) +
          "  |  N=novo  TAB=trocar  X/Y/Z=rotar  WASD/I/J=mover  [/]=escala";
      glfwSetWindowTitle(window, title.c_str());
    }

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    glClearColor(0.08f, 0.11f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspect     = fbH > 0 ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0f;
    const glm::mat4 proj   = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    glUseProgram(program);
    glUniformMatrix4fv(locView,       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(vao);
    for (const auto &t : gCubes)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, t.position);
      model = glm::rotate(model, t.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
      model = glm::rotate(model, t.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
      model = glm::rotate(model, t.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
      model = glm::scale(model, glm::vec3(t.scale));
      glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);

    glfwSwapBuffers(window);
  }

  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteProgram(program);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
