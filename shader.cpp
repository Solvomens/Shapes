/*	Perry Kivolowitz - University of Wisconsin - Madison
Computer Sciences Department

A sample hello world like program demonstrating modern
OpenGL techniques.

Created:	2/25/13
Updates:	9/29/15		Refreshed.

*/

#include "shader.h"
#include <time.h>
#include <assert.h>

using namespace std;
using namespace glm;

#define BAD_GL_VALUE GLuint(-1)

Shader::Shader()
{
	this->is_used = false;
	this->vertex_shader_id = BAD_GL_VALUE;
	this->fragment_shader_id = BAD_GL_VALUE;
	this->program_id = BAD_GL_VALUE;
	srand(unsigned int(time(NULL)));
}

/*	This Shader() class implements or assumes a basic set of uniforms will be
provided to all shaders derived from it. These are listed below.
Shader::CommonSetup() can be used by call derived classes to send the
common values to the shader. Values unique to the derived class can be
loaded with the CustomShader() function.
*/

void Shader::Use()
{
	assert(this->program_id != BAD_GL_VALUE);
	assert(this->is_used == false);
	glUseProgram(this->program_id);
	this->is_used = true;
}

void Shader::UnUse()
{
	assert(this->program_id != BAD_GL_VALUE);
	assert(this->is_used == true);
	glUseProgram(0);
	this->is_used = false;
}

/*	The shader initialization code is lifted liberally from the GLSL 4.0 Cookbook.
*/

bool Shader::Initialize(char * vertex_shader_file, char * fragment_shader_file)
{
	GLint check_value;

	if (GLReturnedError("Shader::Initialize - on entrance"))
		return false;

	this->vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	this->LoadShader(vertex_shader_file, this->vertex_shader_id);
	glCompileShader(this->vertex_shader_id);
	glGetShaderiv(this->vertex_shader_id, GL_COMPILE_STATUS, &check_value);
	if (check_value != GL_TRUE)
	{
		cerr << this->GetShaderLog(vertex_shader_id).str();
		cerr << "GLSL compilation failed - vertex shader: " << vertex_shader_file << endl;
		return false;
	}

	if (GLReturnedError("Shader::Initialize - after processing vertex shader"))
		return false;

	this->fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
	this->LoadShader(fragment_shader_file, this->fragment_shader_id);
	glCompileShader(this->fragment_shader_id);
	glGetShaderiv(this->fragment_shader_id, GL_COMPILE_STATUS, &check_value);
	if (check_value != GL_TRUE)
	{
		cerr << this->GetShaderLog(fragment_shader_id).str();
		cerr << "GLSL compilation failed - fragment shader: " << fragment_shader_file << endl;
		return false;
	}

	this->program_id = glCreateProgram();
	glAttachShader(this->program_id, this->vertex_shader_id);
	glAttachShader(this->program_id, this->fragment_shader_id);
	glLinkProgram(program_id);

	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	return !GLReturnedError("Shader::Initialize - on exit");
}


void Shader::TakeDown()
{
	GLint temp = 0;
	GLsizei size;

	if (this->program_id == (GLuint)-1)
		return;
	glGetProgramiv(this->program_id, GL_ATTACHED_SHADERS, &temp);
	if (temp > 0)
	{
		GLuint * shader_list = new GLuint[temp];
		glGetAttachedShaders(this->program_id, temp, &size, shader_list);

		for (GLsizei i = 0; i < size; i++)
		{
			glDetachShader(this->program_id, shader_list[i]);
			// The shaders were marked for deletion
			// immediately after they were created.
		}
		delete[] shader_list;
	}
	glDeleteProgram(this->program_id);
	this->program_id = (GLuint)-1;
}

/*
This function is adapted from OpenGL 4.0 Shading Language Cookbook by David Wolff.
*/

bool Shader::LoadShader(const char * file_name, GLuint shader_id)
{
	assert(file_name != NULL);
	if (GLReturnedError("Shader::LoadShader - on entrance"))
		return false;

	FILE * file_handle = NULL;
	fopen_s(&file_handle, file_name, "rb");
	if (file_handle == NULL)
	{
		cerr << "Cannot open shader: " << file_name << endl;
		return false;
	}
	fseek(file_handle, 0, SEEK_END);
	int length = ftell(file_handle);
	fseek(file_handle, 0, SEEK_SET);
	GLubyte * buffer = new GLubyte[length + 1];
	fread(buffer, sizeof(GLubyte), length, file_handle);
	fclose(file_handle);
	buffer[length] = 0;
	glShaderSource(shader_id, 1, (const char **)&buffer, NULL);
	delete[] buffer;

	return !GLReturnedError("Shader::LoadShader - on exit");
}

/*
This function is adapted from OpenGL 4.0 Shading Language Cookbook by David Wolff.
*/

stringstream Shader::GetShaderLog(GLuint shader_id)
{
	stringstream s;
	GLint log_length;
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length <= 0)
		s << "No shader log information available." << endl;
	else
	{
		GLchar * buffer = new GLchar[log_length];
		glGetShaderInfoLog(shader_id, log_length, NULL, buffer);
		s << "Shader log:" << endl;
		s << buffer << endl;
		delete[] buffer;
	}
	return s;
}

glm::vec3 Shader::RandomColor(float min , float max)
{
	return vec3(this->Random(min , max) , this->Random(min , max) , this->Random(min , max));
}

bool Shader::GLReturnedError(char * s)
{
	bool return_error = false;

#ifdef _DEBUG
	GLenum glerror;
	while ((glerror = glGetError()) != GL_NO_ERROR)
	{
		return_error = true;
		cerr << s << ": " << gluErrorString(glerror) << endl;
	}
#endif // _DEBUG

	return return_error;
}
