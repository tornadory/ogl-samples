//**********************************
// OpenGL Framebuffer Multisample
// 01/09/2009
//**********************************
// Christophe Riccio
// g.truc.creation@gmail.com
//**********************************
// G-Truc Creation
// www.g-truc.net
//**********************************

#include <glf/glf.hpp>

namespace
{
	std::string const SAMPLE_NAME = "OpenGL Framebuffer Multisample";	
	std::string const VERTEX_SHADER_SOURCE(glf::DATA_DIRECTORY + "330/image-2d.vert");
	std::string const FRAGMENT_SHADER_SOURCE(glf::DATA_DIRECTORY + "330/image-2d.frag");
	std::string const TEXTURE_DIFFUSE(glf::DATA_DIRECTORY + "kueken320-rgb8.tga");
	glm::ivec2 const FRAMEBUFFER_SIZE(320, 240);
	int const SAMPLE_SIZE_WIDTH = 640;
	int const SAMPLE_SIZE_HEIGHT = 480;
	int const SAMPLE_MAJOR_VERSION = 3;
	int const SAMPLE_MINOR_VERSION = 3;

	glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));

	struct vertex
	{
		vertex
		(
			glm::vec2 const & Position,
			glm::vec2 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec2 Position;
		glm::vec2 Texcoord;
	};

	// With DDS textures, v texture coordinate are reversed, from top to bottom
	GLsizei const VertexCount = 6;
	GLsizeiptr const VertexSize = VertexCount * sizeof(vertex);
	vertex const VertexData[VertexCount] =
	{
		vertex(glm::vec2(-4.0f,-3.0f), glm::vec2(0.0f, 1.0f)),
		vertex(glm::vec2( 4.0f,-3.0f), glm::vec2(1.0f, 1.0f)),
		vertex(glm::vec2( 4.0f, 3.0f), glm::vec2(1.0f, 0.0f)),
		vertex(glm::vec2( 4.0f, 3.0f), glm::vec2(1.0f, 0.0f)),
		vertex(glm::vec2(-4.0f, 3.0f), glm::vec2(0.0f, 0.0f)),
		vertex(glm::vec2(-4.0f,-3.0f), glm::vec2(0.0f, 1.0f))
	};

	GLuint VertexArrayName = 0;
	GLuint ProgramName = 0;

	GLuint BufferName = 0;
	GLuint Texture2DName = 0;
	
	GLuint MultisampleTextureName = 0;
	GLuint ColorTextureName = 0;
	
	GLuint FramebufferRenderName = 0;
	GLuint FramebufferResolveName = 0;

	GLuint UniformMVP = 0;
	GLuint UniformDiffuse = 0;	

}//namespace

bool initProgram()
{
	bool Validated = true;
	
	// Create program
	if(Validated)
	{
		ProgramName = glf::createProgram(VERTEX_SHADER_SOURCE, FRAGMENT_SHADER_SOURCE);
		glLinkProgram(ProgramName);
		Validated = glf::checkProgram(ProgramName);
	}

	if(Validated)
	{
		UniformMVP = glGetUniformLocation(ProgramName, "MVP");
		UniformDiffuse = glGetUniformLocation(ProgramName, "Diffuse");
	}

	// Set some variables 
	if(Validated)
	{
		// Bind the program for use
		glUseProgram(ProgramName);

		// Set uniform value
		glUniform1i(UniformDiffuse, 0);

		// Unbind the program
		glUseProgram(0);
	}

	return glf::checkError("initProgram");
}

bool initArrayBuffer()
{
	glGenBuffers(1, &BufferName);
    glBindBuffer(GL_ARRAY_BUFFER, BufferName);
    glBufferData(GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return glf::checkError("initArrayBuffer");;
}

bool initTexture2D()
{
	glGenTextures(1, &Texture2DName);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture2DName);

	// Set image
	gli::image Image = gli::import_as(TEXTURE_DIFFUSE);
	for(std::size_t Level = 0; Level < Image.levels(); ++Level)
	{
		glTexImage2D(
			GL_TEXTURE_2D, 
			GLint(Level), 
			GL_RGB, 
			GLsizei(Image[Level].dimensions().x), 
			GLsizei(Image[Level].dimensions().y), 
			0,  
			GL_RGB, 
			GL_UNSIGNED_BYTE, 
			Image[Level].data());
	}

	return glf::checkError("initTexture2D");
}

bool initFramebuffer()
{
	glGenTextures(1, &MultisampleTextureName);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, MultisampleTextureName);
	// The second parameter is the number of samples.
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, FRAMEBUFFER_SIZE.x, FRAMEBUFFER_SIZE.y, GL_FALSE);

	glGenFramebuffers(1, &FramebufferRenderName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferRenderName);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, MultisampleTextureName, 0);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &ColorTextureName);
	glBindTexture(GL_TEXTURE_2D, ColorTextureName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FRAMEBUFFER_SIZE.x, FRAMEBUFFER_SIZE.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenFramebuffers(1, &FramebufferResolveName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferResolveName);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ColorTextureName, 0);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return glf::checkError("initFramebuffer");
}

bool initVertexArray()
{
	// Create a dummy vertex array object where all the attribute buffers and element buffers would be attached 
	glGenVertexArrays(1, &VertexArrayName);
    glBindVertexArray(VertexArrayName);
		glBindBuffer(GL_ARRAY_BUFFER, BufferName);
		glVertexAttribPointer(glf::semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), GLF_BUFFER_OFFSET(0));
		glVertexAttribPointer(glf::semantic::attr::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), GLF_BUFFER_OFFSET(sizeof(glm::vec2)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(glf::semantic::attr::POSITION);
		glEnableVertexAttribArray(glf::semantic::attr::TEXCOORD);
	glBindVertexArray(0);

	return glf::checkError("initVertexArray");
}

bool begin()
{
	GLint MajorVersion = 0;
	GLint MinorVersion = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &MajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &MinorVersion);
	bool Validated = (MajorVersion * 10 + MinorVersion) >= (SAMPLE_MAJOR_VERSION * 10 + SAMPLE_MINOR_VERSION);

	if(Validated)
		Validated = initProgram();
	if(Validated)
		Validated = initArrayBuffer();
	if(Validated)
		Validated = initVertexArray();
	if(Validated)
		Validated = initTexture2D();
	if(Validated)
		Validated = initFramebuffer();

	return Validated && glf::checkError("begin");
}

bool end()
{
	glDeleteBuffers(1, &BufferName);
	glDeleteProgram(ProgramName);
	glDeleteTextures(1, &Texture2DName);
	glDeleteTextures(1, &ColorTextureName);
	glDeleteTextures(1, &MultisampleTextureName);
	glDeleteFramebuffers(1, &FramebufferRenderName);
	glDeleteFramebuffers(1, &FramebufferResolveName);
	glDeleteVertexArrays(1, &VertexArrayName);

	return glf::checkError("end");
}

void renderFBO(GLuint Framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
	glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::mat4 Perspective = glm::perspective(45.0f, float(FRAMEBUFFER_SIZE.x) / FRAMEBUFFER_SIZE.y, 0.1f, 100.0f);
	glm::mat4 ViewFlip = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f,-1.0f, 1.0f));
	glm::mat4 ViewTranslate = glm::translate(ViewFlip, glm::vec3(0.0f, 0.0f, -Window.TranlationCurrent.y * 2.0));
	glm::mat4 View = glm::rotate(ViewTranslate,-15.f, glm::vec3(0.f, 0.f, 1.f));
	glm::mat4 Model = glm::mat4(1.0f);
	glm::mat4 MVP = Perspective * View * Model;
	glUniformMatrix4fv(UniformMVP, 1, GL_FALSE, &MVP[0][0]);

	glViewport(0, 0, FRAMEBUFFER_SIZE.x, FRAMEBUFFER_SIZE.y);
	glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture2DName);

	glBindVertexArray(VertexArrayName);
	glDrawArrays(GL_TRIANGLES, 0, VertexCount);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glf::checkError("renderFBO");
}

void renderFB(GLuint Texture2DName)
{
	glm::mat4 Perspective = glm::perspective(45.0f, float(Window.Size.x) / Window.Size.y, 0.1f, 100.0f);
	glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -Window.TranlationCurrent.y * 2.0));
	glm::mat4 ViewRotateX = glm::rotate(ViewTranslate, Window.RotationCurrent.y, glm::vec3(1.f, 0.f, 0.f));
	glm::mat4 View = glm::rotate(ViewRotateX, Window.RotationCurrent.x, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 Model = glm::mat4(1.0f);
	glm::mat4 MVP = Perspective * View * Model;
	glUniformMatrix4fv(UniformMVP, 1, GL_FALSE, &MVP[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture2DName);

	glBindVertexArray(VertexArrayName);
	glDrawArrays(GL_TRIANGLES, 0, VertexCount);
		
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glf::checkError("renderFB");
}

void display()
{
	// Clear the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.0f, 0.5f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(ProgramName);

	// Pass 1
	// Render the scene in a multisampled framebuffer
	glEnable(GL_MULTISAMPLE);
	renderFBO(FramebufferRenderName);
	glDisable(GL_MULTISAMPLE);

	// Resolved multisampling
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FramebufferRenderName);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FramebufferResolveName);
	glBlitFramebuffer(
		0, 0, FRAMEBUFFER_SIZE.x, FRAMEBUFFER_SIZE.y, 
		0, 0, FRAMEBUFFER_SIZE.x, FRAMEBUFFER_SIZE.y, 
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Pass 2
	// Render the colorbuffer from the multisampled framebuffer
	glViewport(0, 0, Window.Size.x, Window.Size.y);
	renderFB(ColorTextureName);

	glf::checkError("display");
	glf::swapBuffers();
}

int main(int argc, char* argv[])
{
	if(glf::run(
		argc, argv,
		glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT), 
		::SAMPLE_MAJOR_VERSION, 
		::SAMPLE_MINOR_VERSION))
		return 0;
	return 1;
}
