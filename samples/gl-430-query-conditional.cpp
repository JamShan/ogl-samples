#include "test.hpp"

namespace
{
	char const* VERT_SHADER_SOURCE("gl-430/query-conditional.vert");
	char const* FRAG_SHADER_SOURCE("gl-430/query-conditional.frag");

	GLsizei const VertexCount(6);
	GLsizeiptr const PositionSize = VertexCount * sizeof(glm::vec2);
	glm::vec2 const PositionData[VertexCount] =
	{
		glm::vec2(-1.0f,-1.0f),
		glm::vec2( 1.0f,-1.0f),
		glm::vec2( 1.0f, 1.0f),
		glm::vec2( 1.0f, 1.0f),
		glm::vec2(-1.0f, 1.0f),
		glm::vec2(-1.0f,-1.0f)
	};

	namespace buffer
	{
		enum type
		{
			VERTEX,
			TRANSFORM,
			MATERIAL,
			MAX
		};
	}//namespace buffer
}//namespace

class sample : public framework
{
public:
	sample(int argc, char* argv[]) :
		framework(argc, argv, "gl-430-query-conditional", framework::CORE, 4, 2),
		VertexArrayName(0),
		PipelineName(0),
		ProgramName(0),
		QueryName(0),
		UniformMaterialOffset(0)
	{}

private:
	std::array<GLuint, buffer::MAX> BufferName;
	GLuint VertexArrayName;
	GLuint PipelineName;
	GLuint ProgramName;
	GLuint QueryName;
	GLuint UniformMaterialOffset;

	bool initQuery()
	{
		glGenQueries(1, &QueryName);

		int QueryBits(0);
		glGetQueryiv(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, GL_QUERY_COUNTER_BITS, &QueryBits);

		bool Validated = QueryBits >= 1;

		return Validated && this->checkError("initQuery");
	}

	bool initProgram()
	{
		bool Validated = true;
	
		if(Validated)
		{
			compiler Compiler;
			GLuint VertShaderName = Compiler.create(GL_VERTEX_SHADER, getDataDirectory() + VERT_SHADER_SOURCE, "--version 420 --profile core");
			GLuint FragShaderName = Compiler.create(GL_FRAGMENT_SHADER, getDataDirectory() + FRAG_SHADER_SOURCE, "--version 420 --profile core");
			Validated = Validated && Compiler.check();

			ProgramName = glCreateProgram();
			glProgramParameteri(ProgramName, GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(ProgramName, VertShaderName);
			glAttachShader(ProgramName, FragShaderName);
			glLinkProgram(ProgramName);
			Validated = Compiler.check_program(ProgramName);
		}

		if(Validated)
		{
			glGenProgramPipelines(1, &PipelineName);
			glUseProgramStages(PipelineName, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, ProgramName);
		}

		return Validated && this->checkError("initProgram");
	}

	bool initBuffer()
	{
		GLint UniformBufferOffset(0);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformBufferOffset);

		glGenBuffers(buffer::MAX, &BufferName[0]);

		glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
		glBufferData(GL_ARRAY_BUFFER, PositionSize, &PositionData[0][0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		GLint UniformTransformBlockSize = glm::max(GLint(sizeof(glm::mat4)), UniformBufferOffset);

		glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
		glBufferData(GL_UNIFORM_BUFFER, UniformTransformBlockSize, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		this->UniformMaterialOffset = glm::max(GLint(sizeof(glm::vec4)), UniformBufferOffset);

		glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::MATERIAL]);
		glBufferData(GL_UNIFORM_BUFFER, this->UniformMaterialOffset * 2, nullptr, GL_STATIC_DRAW);

			glm::byte* Pointer = reinterpret_cast<glm::byte*>(
				glMapBufferRange(GL_UNIFORM_BUFFER, 0, this->UniformMaterialOffset * 2, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

			*reinterpret_cast<glm::vec4*>(Pointer + 0) = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
			*reinterpret_cast<glm::vec4*>(Pointer + this->UniformMaterialOffset) = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return this->checkError("initBuffer");
	}

	bool initVertexArray()
	{
		glGenVertexArrays(1, &VertexArrayName);
		glBindVertexArray(VertexArrayName);
			glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
			glVertexAttribPointer(semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glEnableVertexAttribArray(semantic::attr::POSITION);
		glBindVertexArray(0);

		return this->checkError("initVertexArray");
	}

	bool begin()
	{
		bool Validated = true;
		Validated = Validated && this->checkExtension("GL_ARB_ES3_compatibility");

		if(Validated)
			Validated = initQuery();
		if(Validated)
			Validated = initProgram();
		if(Validated)
			Validated = initBuffer();
		if(Validated)
			Validated = initVertexArray();
		if(Validated)
			Validated = initQuery();

		return Validated && this->checkError("begin");
	}

	bool end()
	{
		glDeleteProgramPipelines(1, &PipelineName);
		glDeleteProgram(ProgramName);
		glDeleteBuffers(buffer::MAX, &BufferName[0]);
		glDeleteVertexArrays(1, &VertexArrayName);
		glDeleteQueries(1, &QueryName);

		return this->checkError("end");
	}

	bool render()
	{
		glm::ivec2 WindowSize(this->getWindowSize());

		{
			glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
			glm::mat4* Pointer = reinterpret_cast<glm::mat4*>(glMapBufferRange(GL_UNIFORM_BUFFER,
				0, sizeof(glm::mat4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

			glm::mat4 Projection = glm::perspective(glm::pi<float>() * 0.25f, static_cast<float>(WindowSize.x) / static_cast<float>(WindowSize.y), 0.1f, 100.0f);
			glm::mat4 Model = glm::mat4(1.0f);
		
			*Pointer = Projection * this->view() * Model;

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}

		// Set the display viewport
		glViewport(0, 0, WindowSize.x, WindowSize.y);
		glBindBufferBase(GL_UNIFORM_BUFFER, semantic::uniform::TRANSFORM0, BufferName[buffer::TRANSFORM]);

		// Clear color buffer with black
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)[0]);

		glBindProgramPipeline(PipelineName);
		glBindVertexArray(VertexArrayName);
		glBindBufferRange(GL_UNIFORM_BUFFER, semantic::uniform::MATERIAL, BufferName[buffer::MATERIAL], 0, sizeof(glm::vec4));
	
		// The first orange quad is not written in the framebuffer.
		glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// Beginning of the samples count query
		glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, QueryName);
			// To framework the condional rendering, comment this line, the next draw call won't happen.
			glDrawArraysInstanced(GL_TRIANGLES, 0, VertexCount, 1);
		// End of the samples count query
		glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);

		// The second blue quad is written in the framebuffer only if a sample pass the occlusion query.
		glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	
		glBindBufferRange(GL_UNIFORM_BUFFER, semantic::uniform::MATERIAL, BufferName[buffer::MATERIAL], this->UniformMaterialOffset, sizeof(glm::vec4));

		// Draw only if one sample went through the tests, 
		// we don't need to get the query result which prevent the rendering pipeline to stall.
		glBeginConditionalRender(QueryName, GL_QUERY_WAIT);

			// Clear color buffer with white
			glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f)[0]);

			glDrawArraysInstanced(GL_TRIANGLES, 0, VertexCount, 1);
		glEndConditionalRender();

		return true;
	}
};

int main(int argc, char* argv[])
{
	int Error = 0;

	sample Sample(argc, argv);
	Error += Sample();

	return Error;
}

