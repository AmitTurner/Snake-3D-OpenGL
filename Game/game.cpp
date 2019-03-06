#include "game.h"


Game::Game():Scene(){}

Game::Game(glm::vec3 position,float angle,float hwRelation,float near, float far) : Scene(position,angle,hwRelation,near,far)
{ 

}

void Game::Init()
{
	addCube("",-1);
	//addAxis();

}
void Game::Update( glm::mat4 MVP ,glm::mat4 *jointTransforms,const int length,const int  shaderIndx)
{
	shaders[shaderIndx]->Bind();
	int r = ((pickedShape+1) & 0x000000FF) >>  0;
	int g = ((pickedShape+1) & 0x0000FF00) >>  8;
	int b = ((pickedShape+1) & 0x00FF0000) >> 16;
	shaders[shaderIndx]->SetUniformMat4f("MVP", MVP);
	shaders[shaderIndx]->SetUniformMat4f("Normal", *jointTransforms);
	shaders[shaderIndx]->SetUniform4f("lightDirection", 0.0f, 0.0f, 1.0f,0.0f);
		//glUniform3f(m_uniforms[3], r/255.0f, g/255.0f, b/255.0f);
	shaders[shaderIndx]->SetUniform4f("lightColor",r/255.0f, g/255.0f, b/255.0f,1.0f);
}

Game::~Game(void)
{
}
