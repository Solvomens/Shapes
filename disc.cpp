#include <iostream>
#include <stdexcept>
#include "disc.h"

using namespace std;
using namespace glm;

Disc::Disc(int slices, float span, float outer_radius, float inner_radius) : Shape()
{
	float full_span = pi<float>() * 2.0f;

	if (span == 0)
		throw std::invalid_argument("bad span value");
	
	this->slices = abs(slices);
	this->span = min<float>(full_span, abs(span));
	this->inner_radius = inner_radius;
	this->outer_radius = outer_radius;

	this->is_partial_span = this->span != full_span;
}

bool Disc::PreGLInitialize()
{
	this->is_fan = false;

	float scale_factor_for_normals = 1.0f / 10.0f;
	int real_number_of_slices = this->slices + (this->is_partial_span ? 1 : 0);

	// If the inner radius is greater than zero, two rings of vertices will be created.
	// The inner and outer ring will be connected via two triangles per slice.
	//
	// If the inner radius is zero, a triangle fan will be created.

	this->data.vertices.reserve(real_number_of_slices + (this->inner_radius == 0.0f ? 1 : real_number_of_slices));
	this->data.normals.reserve(real_number_of_slices + (this->inner_radius == 0.0f ? 1 : real_number_of_slices));

	mat4 m;
	vec4 p(this->outer_radius, 0.0f, 0.0f, 1.0f);
	vec3 n(0.0f, 0.0f, 1.0f);
	vec3 z(0.0f, 0.0f, 1.0f);

	float theta = this->span / float(this->slices);

	if (this->inner_radius == 0.0)
	{
		this->is_fan = true;
		// Add center as first point so that triangle fan can be used.
		this->data.vertices.push_back(vec3(0.0f, 0.0f, 0.0f));
		this->data.textures.push_back(vec2(0.0f , 0.0f));
		this->data.colors.push_back(this->RandomColor(vec4(0.5f , 0.5f , 0.5f , 1.0f)));
		this->data.normals.push_back(n);
		this->data.normal_visualization_coordinates.push_back(vec3(0.0f, 0.0f, epsilon<float>()));
		this->data.normal_visualization_coordinates.push_back(n * scale_factor_for_normals);
	}

	// An outer ring is required in all cases.
	for (int i = 0; i < real_number_of_slices; i++)
	{
		this->data.vertices.push_back(vec3(m * p));
		this->data.textures.push_back(vec2(p) / (this->outer_radius * 2.0f) + vec2(0.5f , 0.5f));
		this->data.normals.push_back(n);
		this->data.colors.push_back(this->RandomColor(vec4(0.5f , 0.5f , 0.5f , 1.0f) , -0.3f , 0.3f));
		this->data.normal_visualization_coordinates.push_back(vec3(m * p) + vec3(0.0f, 0.0f, 0.001f));
		this->data.normal_visualization_coordinates.push_back(vec3(m * p) + n * scale_factor_for_normals);
		m = rotate(m, theta, z);
	}

	if (this->inner_radius == 0.0)
	{
		// This is THE only time a TRIANGLE_FAN works.
		for (unsigned int i = 0; i < this->data.vertices.size(); i++)
		{
			this->data.indices.push_back(i);
		}
		if (!this->is_partial_span)
			this->data.indices.push_back(1);
	}
	else
	{
		m = mat4();
		p = vec4(this->inner_radius, 0.0f, 0.0f, 1.0f);

		// Instanciate the second ring.
		for (int i = 0; i < real_number_of_slices; i++)
		{
			this->data.vertices.push_back(vec3(m * p));
			this->data.textures.push_back(vec2(p) / (this->inner_radius * 2.0f) + vec2(0.5f , 0.5f));
			this->data.normals.push_back(n);
			this->data.colors.push_back(this->RandomColor(vec4(0.5f, 0.5f, 0.5f, 1.0f)));
			this->data.normal_visualization_coordinates.push_back(vec3(m * p) + vec3(0.0f, 0.0f, 0.001f));
			this->data.normal_visualization_coordinates.push_back(vec3(m * p) + n * scale_factor_for_normals);
			m = rotate(m, theta, z);
		}

		// There will be two triangles per slice.
		// First is: current outer, current inner, next outer
		// Second is: next outer, current inner, next inner
		for (int i = 0; i < this->slices; i++)
		{
			// First
			this->data.indices.push_back(i);
			this->data.indices.push_back(real_number_of_slices + i);
			this->data.indices.push_back((i + 1) % real_number_of_slices);
			// Second
			this->data.indices.push_back((i + 1) % real_number_of_slices);
			this->data.indices.push_back(real_number_of_slices + i);
			this->data.indices.push_back(real_number_of_slices + (i + 1) % real_number_of_slices);
		}
	}
	this->data.vbackup = this->data.vertices;
	return true;
}

void Disc::NonGLTakeDown()
{

}

void Disc::RecomputeNormals()
{
	vec3 sum;
	float denominator;

	vec3 A;
	vec3 B;
	if (this->inner_radius == 0.0f)
	{
		// Processing for the central vertex only.
		for (unsigned int i = 0; i < this->data.vertices.size() - 1; i++)
		{
			if (i != data.vertices.size() - 2)
			{
				//cross product between each set of verticies and find average
				A = (this->data.vertices[0] - this->data.vertices[i + 2]);
				B = (this->data.vertices[0] - this->data.vertices[i + 1]);
			}
			else if (!this->is_partial_span)
			{
				A = (this->data.vertices[0] - this->data.vertices[1]);
				B = (this->data.vertices[0] - this->data.vertices[i + 1]);
			}
			sum += normalize(cross(normalize(B) , normalize(A)));
		}
		this->data.normals[0] = sum / float(this->data.vertices.size() - (this->is_partial_span ? 2 : 1));
	
		for (unsigned int i = 1; i < this->data.vertices.size(); i++)
		{	
			sum = vec3();
			float points = 0.0f;
			if ((i != this->data.vertices.size() - 1 && is_partial_span) || !is_partial_span)
			{
				A = (this->data.vertices[i] - this->data.vertices[0]);
				B = (this->data.vertices[i] - this->data.vertices[(i == this->data.vertices.size() - 1 ? 1 : i + 1)]);
				sum += normalize(cross(normalize(B) , normalize(A)));
				points++;
			}
			if ((i != 1 && is_partial_span) || !is_partial_span)
			{
				A = (this->data.vertices[i] - this->data.vertices[(i == 1 ? this->data.vertices.size() : i) - 1]);
				B = (this->data.vertices[i] - this->data.vertices[0]);
				sum += normalize(cross(normalize(B) , normalize(A)));
				points++;
			}

			this->data.normals[i] = sum / points;
		}
	}
	else
	{
		// Outer verticies
		for (int i = 0; i < this->slices; i++)
		{
			if (this->is_partial_span)
			{
				// When the disk is a partial span, the first and last
				// slices but be considered differently. 
				if (i == 0)
				{
					// The first outer vertex consists of a single triangle.
					denominator = 1.0f;
					A = normalize(this->data.vertices[i + 1] - this->data.vertices[i]);
					B = normalize(this->data.vertices[i + this->slices] - -this->data.vertices[i]);
					sum = normalize(cross(B , A));
				}
				else
				{
					denominator = 2.0;
					A = normalize(this->data.vertices[i + this->slices - 1] - this->data.vertices[i]);
					B = normalize(this->data.vertices[i - 1] - -this->data.vertices[i]);
					sum = normalize(cross(B , A));
					A = normalize(this->data.vertices[i + this->slices] - this->data.vertices[i]);
					B = normalize(this->data.vertices[i + this->slices - 1] - -this->data.vertices[i]);
					sum += normalize(cross(B , A));
				}
				this->data.normals[i] = sum / denominator;
			}

		}
	}
}

void Disc::Draw(bool draw_normals)
{
	this->GLReturnedError("Disc::Draw() - entering");

	if (this->data.vertices.size() == 0)
	{
		this->PreGLInitialize();
		this->CommonGLInitialization();
	}

	if (draw_normals)
	{
		glBindVertexArray(this->normal_array_handle);
		glDrawArrays(GL_LINES, 0, this->data.normal_visualization_coordinates.size());
	}
	else
	{
		glBindVertexArray(this->vertex_array_handle);
		glDrawElements(this->is_fan ? GL_TRIANGLE_FAN : GL_TRIANGLES, this->data.indices.size(), GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);
	this->GLReturnedError("Disc::Draw() - exiting");
}