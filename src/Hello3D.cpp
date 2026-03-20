#include <iostream>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

namespace
{
  constexpr int kWindowWidth = 960;
  constexpr int kWindowHeight = 540;

  GLuint CompileShader(GLenum shaderType, const char *source)
  {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE)
    {
      GLint logLength = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
      std::string log(logLength, '\0');
      glGetShaderInfoLog(shader, logLength, nullptr, log.data());
      std::cerr << "Erro ao compilar shader: " << log << std::endl;
      glDeleteShader(shader);
      return 0;
    }

    return shader;
  }

  GLuint CreateProgram(const char *vertexSource, const char *fragmentSource)
  {
    const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0)
    {
      return 0;
    }

    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0)
    {
      glDeleteShader(vertexShader);
      return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE)
    {
      GLint logLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
      std::string log(logLength, '\0');
      glGetProgramInfoLog(program, logLength, nullptr, log.data());
      std::cerr << "Erro ao linkar programa: " << log << std::endl;
      glDeleteProgram(program);
      program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
  }
}

int main()
{
  if (!glfwInit())
  {
    std::cerr << "Erro ao inicializar GLFW." << std::endl;
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(kWindowWidth, kWindowHeight, "Ola 3D - Eduardo Kuhn", nullptr, nullptr);
  if (!window)
  {
    std::cerr << "Erro ao criar janela OpenGL." << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    void main()
    {
      gl_Position = vec4(aPos, 0.0, 1.0);
    }
  )";

  const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main()
    {
      FragColor = vec4(0.97, 0.59, 0.22, 1.0);
    }
  )";

  GLuint shaderProgram = CreateProgram(vertexShaderSource, fragmentShaderSource);
  if (shaderProgram == 0)
  {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  const float vertices[] = {
      0.0f,
      0.45f,
      -0.4f,
      -0.3f,
      0.4f,
      -0.3f,
  };

  GLuint vao = 0;
  GLuint vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  while (!glfwWindowShouldClose(window))
  {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    glClearColor(0.08f, 0.11f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteProgram(shaderProgram);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
