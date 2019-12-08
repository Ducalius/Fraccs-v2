#define GLEW_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <glew.h>
#include <glfw3.h>
#include <iostream>

#include <stb_image.h>
#include "shader/shader.h"

int screenWidth = 800;
int screenHeight = 600;

float vertices[] = {
		 1.0f,  1.0f,  0.0f,
		 1.0f, -1.0f,  0.0f,
		-1.0f, -1.0f,  0.0f,
		-1.0f,  1.0f,  0.0f,
};
unsigned int indices[] = {
	0,1,3,
	1,2,3
};

bool msaa = false;
bool ch_text = false;

int maxiter = 50;

double zoom = 1.0;
double center[] = {0.0, 0.0};

double julia_point[] = {0.0, 0.0};

const float move_factor = 0.05;

GLFWwindow* window;

Shader* shader;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	const double zoom_delta = zoom * 0.1f;
	const double move_delta = move_factor / zoom;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
		zoom += zoom_delta;

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
		zoom -= zoom_delta;

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
		center[0] -= move_delta;

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
		center[0] += move_delta;

	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
		center[1] += move_delta;

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
		center[1] -= move_delta;

	if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS))
		maxiter -= maxiter > 0 ? (int)(maxiter * 0.1f) : 0;

	if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS))
		maxiter += (int)(maxiter * 0.1f);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		msaa = !msaa;

}

void window_size_callback(GLFWwindow* window, int l_width, int l_height)
{
	glfwGetWindowSize(window, &screenWidth, &screenHeight);

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

int main(int argc, char *argv[])
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);


	window = glfwCreateWindow(screenWidth, screenHeight, "Set Viewer", nullptr, nullptr);

	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwSetKeyCallback(window, key_callback);

	glfwSetWindowSizeCallback(window, window_size_callback);

	shader = new Shader("shaders/vertex.vert", "shaders/fragment.frag");

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	int tWidth, tHeight, nrChannels;

	unsigned int texture;

	if (argc > 1) {

		
		stbi_set_flip_vertically_on_load(true);

		glGenTextures(0, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned char *data = stbi_load(argv[1], &tWidth, &tHeight, &nrChannels, 0);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tWidth, tHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
		}
		stbi_image_free(data);

		shader->setInt("texture2", 0);
	}
	shader->use();
	
	while (!glfwWindowShouldClose(window)) {
		
		// render
		// ------

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shader->setFloat("maxiter", (float)maxiter);
		shader->setVec2("resolution", (float)screenHeight, (float)screenWidth);
		glUniform2d(glGetUniformLocation(shader->ID, "julia_point"), julia_point[0], julia_point[1]);
		glUniform1d(glGetUniformLocation(shader->ID, "zoom"), zoom);
		glUniform2d(glGetUniformLocation(shader->ID, "center"), center[0], center[1]);
		shader->setBool("msaa", msaa);

		if (argc > 1) {
			shader->setBool("trap_bitmap", true);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
		}
		
		shader->use();
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwWaitEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();
	return 0;
}



