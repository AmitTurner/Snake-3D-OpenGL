#pragma once
#include "../KDtree/intersect.h"
#include "levelParser.h"
#include "shape.h"
#include "listNode.h"

struct levelIntersect {
	float x;
	float y;
	int level;
	Shape *myShape;
	void(*onIntersect)(std::vector<IndexedModel> sol);
	intersect *model;
};

static const float radiusLenToCheckIntersects = 1.5f*allscale;
void addObj(float x, float y, int level, Shape *myShape, void(*onIntersect)(std::vector<IndexedModel> sol), std::vector<glm::vec3> &shape);
void addSnakeHead(std::vector<glm::vec3> &shape);
void isIntersectSnakeHead(glm::mat4 tranSnake, float x, float y, int level);
void printDSDebug();