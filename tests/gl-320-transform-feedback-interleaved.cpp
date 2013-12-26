//**********************************
// OpenGL Transform Feedback interleaved
// 06/04/2010 - 23/02/2013
//**********************************
// Christophe Riccio
// ogl-samples@g-truc.net
//**********************************
// G-Truc Creation
// www.g-truc.net
//**********************************

#include <glf/glf.hpp>

namespace
{
	char const * SAMPLE_NAME("OpenGL Transform Feedback Interleaved");
	char const * VERT_SHADER_SOURCE_TRANSFORM("gl-320/transform-feedback-transform.vert");
	char const * VERT_SHADER_SOURCE_FEEDBACK("gl-320/transform-feedback-feedback.vert");
	char const * FRAG_SHADER_SOURCE_FEEDBACK("gl-320/transform-feedback-feedback.frag");
	int const SAMPLE_SIZE_WIDTH(640);
	int const SAMPLE_SIZE_HEIGHT(480);
	int const SAMPLE_MAJOR_VERSION(3);
	int const SAMPLE_MINOR_VERSION(2);

	glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));

	GLsizei const VertexCount(6);
	GLsizeiptr const PositionSize = VertexCount * sizeof(glm::vec4);
	glm::vec4 const PositionData[VertexCount] =
	{
		glm::vec4(-1.0f,-1.0f, 0.0f, 1.0f),
		glm::vec4( 1.0f,-1.0f, 0.0f, 1.0f),
		glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(-1.0f,-1.0f, 0.0f, 1.0f)
	};

	namespace program
	{
		enum type
		{
			TRANSFORM,
			FEEDBACK,
			MAX
		};
	}//namespace program

	namespace shader
	{
		enum type
		{
			VERT_TRANSFORM,
			VERT_FEEDBACK,
			FRAG_FEEDBACK,
			MAX
		};
	}//namespace shader

	std::vector<GLuint> ShaderName(shader::MAX);
	std::vector<GLuint> ProgramName(program::MAX);
	std::vector<GLuint> VertexArrayName(program::MAX);
	std::vector<GLuint> BufferName(program::MAX);
	GLint TransformUniformMVP(0);
	GLint FeedbackUniformDiffuse(0);
	GLuint Query(0);
}//namespace

bool initDebugOutput()
{
#	ifdef GL_ARB_debug_output
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageCallbackARB(&glf::debugOutput, NULL);
#	endif

	return true;
}

bool initProgram()
{
	bool Validated = true;
	
	glf::compiler Compiler;
	ShaderName[shader::VERT_TRANSFORM] = Compiler.create(GL_VERTEX_SHADER, glf::DATA_DIRECTORY + VERT_SHADER_SOURCE_TRANSFORM, "--version 150 --profile core");
	ShaderName[shader::VERT_FEEDBACK] = Compiler.create(GL_VERTEX_SHADER, glf::DATA_DIRECTORY + VERT_SHADER_SOURCE_FEEDBACK, "--version 150 --profile core");
	ShaderName[shader::FRAG_FEEDBACK] = Compiler.create(GL_FRAGMENT_SHADER, glf::DATA_DIRECTORY + FRAG_SHADER_SOURCE_FEEDBACK, "--version 150 --profile core");
	Validated = Validated && Compiler.check();

	if(Validated)
	{
		ProgramName[program::TRANSFORM] = glCreateProgram();
		glAttachShader(ProgramName[program::TRANSFORM], ShaderName[shader::VERT_TRANSFORM]);
		glBindAttribLocation(ProgramName[program::TRANSFORM], glf::semantic::attr::POSITION, "Position");

		GLchar const * Strings[] = {"gl_Position", "block.Color"}; 
		glTransformFeedbackVaryings(ProgramName[program::TRANSFORM], 2, Strings, GL_INTERLEAVED_ATTRIBS); 
		glLinkProgram(ProgramName[program::TRANSFORM]);

		Validated = Validated && glf::checkProgram(ProgramName[program::TRANSFORM]);

		char Name[64];
		memset(Name, 0, 64);
		GLsizei Length(0);
		GLsizei Size(0);
		GLenum Type(0);

		glGetTransformFeedbackVarying(
			ProgramName[program::TRANSFORM],
			0,
			64,
			&Length,
			&Size,
			&Type,
			Name);

		Validated = Validated && (Size == 1) && (Type == GL_FLOAT_VEC4);
	}

	// Get variables locations
	if(Validated)
	{
		TransformUniformMVP = glGetUniformLocation(ProgramName[program::TRANSFORM], "MVP");

		GLint ActiveUniforms(0);
		glGetProgramiv(ProgramName[program::TRANSFORM], GL_ACTIVE_UNIFORMS, &ActiveUniforms);

		char Name[64];
		memset(Name, 0, 64);
		GLsizei Length(0);
		GLsizei Size(0);
		GLenum Type(0);

		for(GLint i = 0; i < ActiveUniforms; ++i)
		{
			glGetActiveUniform(
				ProgramName[program::TRANSFORM],
				i,
				64,
				&Length,
				&Size,
				&Type,
				Name);

			GLint Location = glGetUniformLocation(ProgramName[program::TRANSFORM], Name);

			if(TransformUniformMVP == Location)
			{
				Validated = Validated && (Size == 1) && (Type == GL_FLOAT_MAT4);
				Validated = Validated && (TransformUniformMVP >= 0);
			}
		}
	}

	// Create program
	if(Validated)
	{
		ProgramName[program::FEEDBACK] = glCreateProgram();
		glAttachShader(ProgramName[program::FEEDBACK], ShaderName[shader::VERT_FEEDBACK]);
		glAttachShader(ProgramName[program::FEEDBACK], ShaderName[shader::FRAG_FEEDBACK]);

		glBindAttribLocation(ProgramName[program::FEEDBACK], glf::semantic::attr::POSITION, "Position");
		glBindAttribLocation(ProgramName[program::FEEDBACK], glf::semantic::attr::COLOR, "Color");
		glBindFragDataLocation(ProgramName[program::FEEDBACK], glf::semantic::frag::COLOR, "Color");
		glLinkProgram(ProgramName[program::FEEDBACK]);
		Validated = Validated && glf::checkProgram(ProgramName[program::FEEDBACK]);
	}

	return Validated && glf::checkError("initProgram");
}

bool initVertexArray()
{
	glGenVertexArrays(program::MAX, &VertexArrayName[0]);

	// Build a vertex array object
	glBindVertexArray(VertexArrayName[program::TRANSFORM]);
		glBindBuffer(GL_ARRAY_BUFFER, BufferName[program::TRANSFORM]);
		glVertexAttribPointer(glf::semantic::attr::POSITION, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(glf::semantic::attr::POSITION);
	glBindVertexArray(0);

	// Build a vertex array object
	glBindVertexArray(VertexArrayName[program::FEEDBACK]);
		glBindBuffer(GL_ARRAY_BUFFER, BufferName[program::FEEDBACK]);
		glVertexAttribPointer(glf::semantic::attr::POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v4fc4f), 0);
		glVertexAttribPointer(glf::semantic::attr::COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v4fc4f), GLF_BUFFER_OFFSET(sizeof(glm::vec4)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(glf::semantic::attr::POSITION);
		glEnableVertexAttribArray(glf::semantic::attr::COLOR);
	glBindVertexArray(0);

	return glf::checkError("initVertexArray");
}

bool initBuffer()
{
	// Generate a buffer object
	glGenBuffers(program::MAX, &BufferName[0]);

	glBindBuffer(GL_ARRAY_BUFFER, BufferName[program::TRANSFORM]);
	glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, BufferName[program::FEEDBACK]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glf::vertex_v4fc4f) * VertexCount, NULL, GL_STATIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return glf::checkError("initBuffer");
}

bool begin()
{
	bool Validated = true;
	Validated = Validated && glf::checkGLVersion(SAMPLE_MAJOR_VERSION, SAMPLE_MINOR_VERSION);

	glGenQueries(1, &Query);

	if(Validated && glf::checkExtension("GL_ARB_debug_output"))
		Validated = initDebugOutput();
	if(Validated)
		Validated = initProgram();
	glf::checkError("initProgram Apple workaround");
	if(Validated)
		Validated = initBuffer();
	if(Validated)
		Validated = initVertexArray();

	return Validated && glf::checkError("begin");
}

bool end()
{
	for(std::size_t i = 0; 0 < shader::MAX; ++i)
		glDeleteShader(ShaderName[i]);
	for(std::size_t i = 0; i < program::MAX; ++i)
		glDeleteProgram(ProgramName[i]);
	glDeleteVertexArrays(program::MAX, &VertexArrayName[0]);
	glDeleteBuffers(program::MAX, &BufferName[0]);
	glDeleteQueries(1, &Query);

	return glf::checkError("end");
}

void display()
{
	// Compute the MVP (Model View Projection matrix)
	glm::mat4 Projection = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f, 0.1f, 100.0f);
	glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -Window.TranlationCurrent.y));
	glm::mat4 ViewRotateX = glm::rotate(ViewTranslate, Window.RotationCurrent.y, glm::vec3(1.f, 0.f, 0.f));
	glm::mat4 View = glm::rotate(ViewRotateX, Window.RotationCurrent.x, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 Model = glm::mat4(1.0f);
	glm::mat4 MVP = Projection * View * Model;

	// Set the display viewport
	glViewport(0, 0, Window.Size.x, Window.Size.y);

	// Clear color buffer
	glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)[0]);

	// First draw, capture the attributes
	// Disable rasterisation, vertices processing only!
	glEnable(GL_RASTERIZER_DISCARD);

	glUseProgram(ProgramName[program::TRANSFORM]);
	glUniformMatrix4fv(TransformUniformMVP, 1, GL_FALSE, &MVP[0][0]);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, BufferName[program::FEEDBACK]); 
	glBindVertexArray(VertexArrayName[program::TRANSFORM]);

	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, Query); 
	glBeginTransformFeedback(GL_TRIANGLES);
		glDrawArraysInstanced(GL_TRIANGLES, 0, VertexCount, 1);
	glEndTransformFeedback();
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN); 

	glDisable(GL_RASTERIZER_DISCARD);

	// Second draw, reuse the captured attributes
	glUseProgram(ProgramName[program::FEEDBACK]);

	GLuint PrimitivesWritten = 0;
	glGetQueryObjectuiv(Query, GL_QUERY_RESULT, &PrimitivesWritten);

	glBindVertexArray(VertexArrayName[program::FEEDBACK]);
	glDrawArraysInstanced(GL_TRIANGLES, 0, PrimitivesWritten * 3, 1);

	glf::swapBuffers();
	glf::checkError("display");
}

int main(int argc, char* argv[])
{
	return glf::run(
		argc, argv,
		glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT), 
		glf::CORE,
		::SAMPLE_MAJOR_VERSION, ::SAMPLE_MINOR_VERSION);
}
