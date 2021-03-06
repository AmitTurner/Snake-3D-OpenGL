#include "game.h"
#include "bezier1D.h"
#include "Bezier2D.h"
#include "MeshConstructor.h"
#include "IntersectTracker.h"
#include "cameraMotion.h"
#include <windows.h>
#include <iostream> 
#include <thread>

IntersectTracker *IT;
Game::Game(vec3 position,float angle,float hwRelation,float near1, float far1): 
	Scene(position,angle,hwRelation,near1,far1){
	IT = new IntersectTracker(this);
	lGen = new leveGenerator(currentLvl);
	sMT = new snakeMoveTracker(snakeLength, speed);
}

Game::~Game(void) {
	delete sMT;
	delete lGen;
	delete IT;
	delete themes;
}

void Game::addShape(IndexedModel model, int parent, unsigned int mode, int tex, int shader)
{
	chainParents.push_back(parent);
	shapes.push_back(new Shape(model, mode, tex, shader));
}

void Game::addShape(int type,int parent,unsigned int mode)
{
	chainParents.push_back(parent);
	if (type != BezierLine && type != BezierSurface)
		shapes.push_back(new Shape(type, mode));
	else
		printf("Bezier1d/2d direct adding is not supported any more\n");
}

void Game::updateDrawMode(unsigned int mode){
	for(unsigned int i = 1; i < shapes.size(); i++)
		shapes[i]->mode= mode;
}

const float jumpy = 0.4f, jumpx = 0.38f;
float lastYext;
float lastY;
static const int bezierRes = 12, cirSubdiv = 4;
inline void Game::getSegsHelper(std::vector<glm::mat4>& segments, float& lastX, float mult, const float sign, const float jumpX, const float jumpY, const int segs) {
	lastY = 0;
	mat4 seg0 = mat4(0);
	float segPart = 1 / float(segs);
	for (int i = 0; i < segs; i++) {
		seg0[0] = vec4(lastX, lastY, 0, 1);
		lastX = lastX + jumpX;	lastY = lastY + jumpY*mult;
		seg0[1] = vec4(lastX, lastY, 0, 1);
		lastX = lastX + jumpX;	lastY = lastY + jumpY*mult;
		seg0[2] = vec4(lastX, lastY, 0, 1);
		lastX = lastX + jumpX;	lastY = lastY + jumpY*mult;
		seg0[3] = vec4(lastX, lastY, 0, 1);
		mult += sign * segPart;
		segments.push_back(seg0);
	};
}

void Game::getSegs(float& lastX, float mult, const float sign, const float jumpX, const float jumpY, const int segs) {
	std::vector<glm::mat4> segments;
	getSegsHelper(segments, lastX, mult, sign, jumpX, jumpY, segs);
	Bezier1D body(segments);
	vec3 axisFrom = *(body.GetControlPoint(0, 0).GetPos());
	Bezier2D b(body, cirSubdiv, yAx, vec3(0, axisFrom.y, 0));
	addShape(b.GetSurface(bezierRes, bezierRes), -1, TRIANGLES, themes->getTex(4), 1);
	orderSegPart(lastY);
}

void Game::orderSegPart(const float segLen) {
	if (pickedShape == -1) {
		lastYext = segLen;
		return;
	}
	shapeTransformation(yGlobalTranslate, lastYext);
	snakeFullLength += segLen;
	lastYext = segLen;
	pickedShape++;
}

void Game::getTailSegs(float& lastX, const float jumpX, const float jumpY, const int segs) {
	getSegs(lastX, 0, 1, jumpX, jumpY, segs);
}

void Game::getBodySegs(float& lastX, const float jumpX, const float jumpY, const int segs, const int amount) {
	getSegs(lastX, 1, 0, jumpX, jumpY, segs);
	Shape *bodySeg = shapes.back();
	for (int i = 0; i < amount - 1; i++) {
		chainParents.push_back(-1);
		shapes.push_back(new Shape(*bodySeg, TRIANGLES));
		orderSegPart(lastYext);
	}
}

void Game::genTongue(int pa) {
	float x = .4f;
	snakeTongue = shapes.size();
	getSegs(x, 1, 0, 0.f, .3f, ends);
	shapes.back()->SetTexture(1);
	shapes.back()->SetShader(4);
	chainParents.back() = pa;
	float lY = lastYext;
	//getSegs(x, 0, 1, 0.f, 0.2f, ends);
	//getSegs(x, 0, 1, 0.1f * float(segs) / ends, 0.2f, segs);	x = .2f;
	getSegs(x, 1 - 1 / float(segs), -1, -0.09f*float(segs) / ends, 0.5f, segs); x = .4f;
	pickedShape = shapes.size() - 1;
	shapeTransformation(yGlobalTranslate, lY);	pickedShape = -1;
	shapes.back()->SetTexture(1);
	shapes.back()->SetShader(4);
	shapes.back()->myRotate(-60.f, zAx, zAxis1);
	chainParents.back() = snakeTongue;
	getSegs(x, 1 - 1 / float(segs), -1, -0.09f*float(segs) / ends, 0.5f, segs);
	pickedShape = shapes.size() - 1;
	shapeTransformation(yGlobalTranslate, lY);	pickedShape = -1;
	shapes.back()->SetTexture(1);
	shapes.back()->SetShader(4);
	shapes.back()->myRotate(60.f, zAx, zAxis1);
	chainParents.back() = snakeTongue;
}

void Game::genEyes(float width, int pa) {
	IndexedModel im = Bezier2D::genBall(5, 5, 8);
	IT->addSnakeHead(MeshConstructor::getlastInitMeshPositions());

	addShape(im, pa, TRIANGLES, 0, 4);//using the basic shader
	shapes.back()->myTranslate(vec3(-width * 0.66f, lastYext * 0.33f, 0), 0);
	shapes.back()->myScale(vec3(2.f));
	shapes.back()->myRotate(90.f, xAx, xAxis1);
	shapes.back()->myRotate(90.f, xAx, zAxis1);
	shapes.push_back(new Shape(*shapes.back(), TRIANGLES));	chainParents.push_back(pa);
	shapes.back()->myTranslate(vec3(width * 0.66f, lastYext * 0.33f, 0), 0);
	shapes.back()->myScale(vec3(2.f));
	shapes.back()->myRotate(90.f, xAx, xAxis1);
	shapes.back()->myRotate(-90.f, xAx, zAxis1);
}

void Game::getHeadSegs(float& lastX, const float jumpX, const float jumpY, const int segs) {
	snakeNodesShapesEnd = shapes.size();
	int pa = snakeNodesShapesEnd;
	float xCopy = lastX;
	getSegs(lastX, 1 - 1 / float(segs), -1, jumpX, jumpY, segs);
	pickedShape = -1;
	genEyes(xCopy, snakeNodesShapesEnd);
	genTongue(snakeNodesShapesEnd);
}

vec3 myDir(int direction) {
	if (direction == 0)
		return yAx;
	if (direction == 2)
		return -yAx;
	if (direction == 1)
		return xAx;
	//if (direction == 3)
	return -xAx;
}

int Game::findMyDir() {
	bool whosBigger = abs(headDirection.x) > abs(headDirection.y);
	if (whosBigger) {
		if (headDirection.x > 0)
			return 1;
		return 3;
	}
	if (headDirection.y > 0)
		return 2;
	return 0;	
}

void Game::putSnakeInPlace(float xLoc, float yLoc, float zLoc, int direction){
	if (direction == 0)
		shapeTransformation(snakeNodesShapesStart, GlobalTranslate, vec3(0, snakeFullLength, 0));
	else if (direction == 1)
		shapeTransformation(snakeNodesShapesStart, GlobalTranslate, vec3(-snakeFullLength, 0, 0));
	else if (direction == 2)
		shapeTransformation(snakeNodesShapesStart, GlobalTranslate, vec3(0, -snakeFullLength, 0));
	else// if (direction == 3)
		shapeTransformation(snakeNodesShapesStart, GlobalTranslate, vec3(snakeFullLength, 0, 0));
	shapeTransformation(snakeNodesShapesStart, GlobalTranslate, vec3(xLoc + allscale * .5f, yLoc + allscale * .5f, zLoc + 5.f));
	shapes[snakeNodesShapesStart]->myRotate(180.f + 90.f * direction, zAx, zAxis1);
}

/*The vertebral column of a snake consists of anywhere between 200 and 400 (or more) vertebrae.*/
void Game::genSnake(float xLoc, float yLoc, float zLoc, int direction) {
	std::vector<Bezier2D> b1vec;
	lastYext = 0; snakeFullLength = 0;
	float x = 0; float rounding = float(segs) / ends;

	snakeNodesShapesStart = shapes.size();
	pickedShape = snakeNodesShapesStart;//chaining!

	getTailSegs(x, jumpx * rounding, 2.f*jumpy, ends);
	getBodySegs(x, 0, jumpy, 4, snakeLength -2);
	//round head generated here
	getHeadSegs(x, -jumpx * rounding, 2.5f*jumpy, ends);
	
	//big obj head
	//addShapeFromFile("../res/objs/snake_head.obj", -1, TRIANGLES, themes->getTex(4), 4);
	//orderGenObj(vec3(0, 0, 0), 0.02f * allscale, (direction + 2)%4);

	for (int i = snakeNodesShapesStart + 1; i <= snakeNodesShapesEnd; i++) {
		setParent(i, i-1);
	}
	
	putSnakeInPlace(xLoc, yLoc, zLoc, direction);
}

char		 **filePath;
IndexedModel **uploadedFiles;
void		 **computedKDtrees;
inline void Game::updateThemeArrays()
{
	theme *tempTheme = themes->getCurrentTheme();
	filePath	    = tempTheme->filepath;
	uploadedFiles   = (IndexedModel **) tempTheme->uploadedObj;
	computedKDtrees = (void  **)		tempTheme->computedKDtrees;
}

void Game::loadThemes() {
	themes = new ThemeHolder(this, 4, currentTheme);
	updateThemeArrays();
}

void Game::changeTheme() {
	themes->swapThemes(currentTheme);
	updateThemeArrays();
}

inline void Game::orderGenObj(vec3 startLoc, float scale, int direction) {
	shapes.back()->myTranslate(startLoc, 0);
	if (scale != -1)
		shapes.back()->myScale(vec3(scale));
	shapes.back()->myRotate(90.f * direction, zAx, zAxis1);
}

void Game::genObj(int ptrIndx, int tex, vec3 startLoc, float scale, int direction) {
	if (uploadedFiles[ptrIndx] == nullptr) {
		addShapeFromFile(filePath[ptrIndx], -1, TRIANGLES, tex, 4);//using the basic shader
		uploadedFiles[ptrIndx] = new IndexedModel(MeshConstructor::getlastIndexedModel());
	}
	else {
		chainParents.push_back(-1);
		shapes.push_back(new Shape(*uploadedFiles[ptrIndx], TRIANGLES, tex, 4));
	}
	orderGenObj(startLoc, scale, direction);
}

bool Game::onIntersectCave(Shape *s) {
	printf("reach to seafty END Level\n");
	if (fruitCounter == 0) {
		isLoading = true;
		PlaySoundGame(Win);
		loadNextLevel();
		return 1;
	}
	resetSnake();
	PlaySoundGame(Hiss);
	Deactivate();
	return 0;
}

void Game::onIntersectFruit(Shape *s) {
	printf("got fruit\n");
	s->Hide();
	IT->remove(s);
	PlaySoundGame(FruitSound);
	superSpeedTicks = 30;
	fruitCounter--;
}

void Game::onIntersectObstecle(Shape *s) {
	printf("bump into obstecle\n");
	PlaySoundGame(ObstecleSound);
	resetCurrentLevel();
	Deactivate();
}

void Game::onIntersectWalls(Shape *s) {
	printf("wall  \n");
	PlaySoundGame(ObstecleSound);
	resetCurrentLevel();
	Deactivate();
}

/*
true - is up
false- is down
*/
void Game::turnSnakeHeadUpDown(bool dir, float angle) {
	printf("turning head %d %f\n", dir, angle);
	int sign = (dir ? 1 : -1);
	//vec3 turnOnMe = glm::cross(zAx, headDirection);
	sMT->add(xAx, sign*angle);
	shapes[snakeNodesShapesEnd]->myRotate(sign*angle, xAx, 4);
}

int fallWallCoolDown = 0;
void Game::onIntersectFallWall(Shape *s) {
	printf("fall wall\n");
	if (fallWallCoolDown > 0) return;
	fallWallCoolDown = 1;
	turnSnakeHeadUpDown(false, 90.f);
}

bool onStair = false;
int stairIntersCoolDown = 0;
Shape *stairWall = nullptr;

void Game::gotToDestinationStair(Shape *s)
{
	onStair = false;
	vec4 wallLoc = s->makeTrans()[3];
	snakeLevel = (int)(wallLoc.z / zscale);
	stairWall = nullptr;
	sMT->flush();
	for (int pShape = snakeNodesShapesStart; pShape <= snakeNodesShapesEnd; pShape++) {
		shapes[pShape]->resetEuler();
		shapes[pShape]->doRotate(mat4(1));
	}
	//shapes[snakeNodesShapesStart]->makeRot(headDirection);
	shapes[snakeNodesShapesStart]->doTranslate(mat4(1), 0);
	shapes[snakeNodesShapesStart]->doTranslate(mat4(1), 1);
	printVec(wallLoc);
	putSnakeInPlace(wallLoc.x, wallLoc.y, wallLoc.z, findMyDir());
}

void Game::onIntersectStairs(Shape *s) {
	printf("stairs %d\n", stairIntersCoolDown);
	if (stairIntersCoolDown > 0) {
		stairIntersCoolDown++;
		return;
	}
	printf("stairs\n");
	printf("%d %d %p %d\n", stairIntersCoolDown, onStair, stairWall, snakeLevel);
	stairIntersCoolDown = 5;
	if (s != stairWall) {//enter stair
		if (onStair)//exit stair from next level
			gotToDestinationStair(s);		
		else{
			onStair = true;
			stairWall = s;
			turnSnakeHeadUpDown(true, climbAngle);//todo how we know which wall we are?
		}
	}
	else//exit stair from where we entered
		gotToDestinationStair(s);
	printf("%d %d %p %d\n", stairIntersCoolDown, onStair, stairWall, snakeLevel);
}

enum MapObjTypes { NOTUSED, Snake, Cave, Obstecle, Fruit };
enum IntersectFuncTypes { CaveF, SnakeGirl, ObstecleF, FruitF, StairF, WallF, FallWallF };
inline void Game::addShapeAndKD(int myIndex, int tex, float x, float y, vec3 pos, int level, float scale, int dir) {
	genObj(myIndex, tex, pos, scale, dir);
	if (computedKDtrees[myIndex])
		IT->addObj(x, y, level, shapes.back(), myIndex, computedKDtrees[myIndex]);
	else
		computedKDtrees[myIndex] = IT->addObj(x, y, level, shapes.back(), myIndex, MeshConstructor::getlastInitMeshPositions());
}

/*
direction map
	0
3		1
	2
*/
void Game::specialObjHandle(objLocation &obj) {
	float x = obj.x, y = obj.y, z = obj.z;
	int dir = obj.direction;
	//printf("%f %f %f %d %d\n", x, y, z, dir, obj.type);
	switch (obj.type) {
	case Snake:
		genSnake(x, y, z, dir);
		snakeLevel = obj.level;
		printf("added Snake\n");
		break;
	case Cave:
		addShapeAndKD(CaveF, themes->getTex(1), x, y, vec3(x + allscale * 0.5f, y + allscale * 0.5f, z - 60.f), obj.level, 0.2f * allscale, dir);
		genObj(1, 18, vec3(x + allscale * 1.f, y, z + 1.f), 0.06f * allscale, (dir+2)%4);
		break;
	case Obstecle:
		addShapeAndKD(ObstecleF, themes->getTex(2), x, y, vec3(x + allscale * 0.5f, y + allscale * 0.5f, z), obj.level, themes->getScale(0) * allscale, dir);
		break;
	case Fruit:
		fruitCounter++;
		addShapeAndKD(FruitF, themes->getTex(3), x, y, vec3(x + allscale * 0.5f, y + allscale * 0.5f, z), obj.level, themes->getScale(1) * allscale, dir);
		fruitsVec.push_back(shapes.back());
		break;
	default:
		printf("unknown special obj <%d>\n", obj.type);
		break;
	}
}

//return true if map objects had changed
bool Game::onIntersectSnakeHead(int type, Shape *myShape) {
	switch (type) {
		case WallF:
			Game::onIntersectWalls(myShape);
			break;
		case FallWallF:
			Game::onIntersectFallWall(myShape);
			break;
		case CaveF:
			return Game::onIntersectCave(myShape);
			break;
		case ObstecleF:
			Game::onIntersectObstecle(myShape);
			break;
		case FruitF:
			Game::onIntersectFruit(myShape);
			break;
		case StairF:
			Game::onIntersectStairs(myShape);
			break;
		default:
			printf("unknown special obj <%d>\n", type);
			break;
	}
	return 0;
};

void Game::addCubes() {
	addShape(Cube, -1, TRIANGLES);
	addShape(Cube, -1, TRIANGLES);
	addShape(Cube, -1, TRIANGLES);
	pickedShape = shapes.size() - 1;
	this->shapeTransformation(this->xGlobalTranslate, 10.f);
	pickedShape--;
	this->shapeTransformation(this->yGlobalTranslate, 20.f);
	pickedShape--;
	this->shapeTransformation(this->zGlobalTranslate, 30.f);
}

//PLAYING THEME MUSIC
void Game::configSound() {
	//PlaySoundGame(Theme);
}

void Game::PlaySoundGame(int type)
{
	if (soundEnable)
		switch (type) {
			case ObstecleSound:
				PlaySound("../res/sounds/explosion.wav", NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT | SND_RING);
			break;
			case FruitSound:
				PlaySound("../res/sounds/eat_apple.wav", NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT | SND_RING);
			break;
			case Win:
				PlaySound("../res/sounds/Cheering.wav", NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT | SND_RING);
			break;
			case Hiss:
				PlaySound("../res/sounds/snakehiss.wav", NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT | SND_RING);
			break;
			case Theme:
				PlaySound("../res/sounds/theme.wav", NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);
			break;

		}
}

void Game::updateSnakePosition()
{
	int pShape = snakeNodesShapesStart;
	mat4 root = shapes[pShape++]->makeTrans();
	tailDirection = Bezier1D::v4to3(root * vec4(0, 1, 0, 0));

	for (; pShape < snakeNodesShapesStart + snakeLength/2; pShape++)
		root *= shapes[pShape]->makeTrans();
	midCurLocation = Bezier1D::v4to3(root[3]);
	for (; pShape < snakeNodesShapesStart + snakeLength; pShape++)
		root *= shapes[pShape]->makeTrans();

	headCurLocation = Bezier1D::v4to3(root[3]);
	headDirection = Bezier1D::v4to3(root * vec4(0, 1, 0, 0));
	headTransMAT = root;
}

inline void Game::orderCameraTop() {
	updateSnakePosition();
	setCameraTopView();
}

void Game::setUpCamera() {
	float zView = -1.5f*snakeFullLength;
	initCameraMotion(this, abs(zView));
	orderCameraTop();
}

void Game::resetSnake() {
	struct objMap map = lGen->getLevel(currentLvl);
	
	onStair = false;
	stairIntersCoolDown = 0;
	stairWall = nullptr;

	sMT->flush();
	for (int pShape = snakeNodesShapesStart; pShape <= snakeNodesShapesEnd; pShape++) {
		shapes[pShape]->resetEuler();
		shapes[pShape]->doRotate(mat4(1));
	}

	bool notfound = 1;	int i = 0;
	while (notfound) {
		struct objLocation obj = (*map.specialObj)[i++];
		switch (obj.type) {
			case Snake:
				shapes[snakeNodesShapesStart]->doTranslate(mat4(1), 0);
				shapes[snakeNodesShapesStart]->doTranslate(mat4(1), 1);
				putSnakeInPlace(obj.x, obj.y, obj.z, obj.direction);
				snakeLevel = (int) (obj.z / zscale);
				notfound = 0;
				break;
		}
	}

	orderCameraTop();
}

void Game::resetCurrentLevel(){

	fruitCounter = fruitsVec.size();
	for (int i = 0; i < (signed)fruitsVec.size(); i++) {
		if (!fruitsVec[i]->Is2Render()) {
			fruitsVec[i]->Unhide();
			vec4 obj = fruitsVec[i]->makeTransScale()[3];
			int level = (int)(obj.z / zscale);
			if (!IT->exist(level, fruitsVec[i])) {
				IT->addObj(obj.x, obj.y, level, fruitsVec[i], FruitF, computedKDtrees[FruitF]);
			}
		}
	}

	resetSnake();
}

void Game::prepareLevel() {
	for (int i = 0; i < (signed)shapes.size(); i++) {
		delete shapes[i];
	}
	chainParents.clear();
	shapes.clear();
	sMT->flush();
	IT->flush();
	setupCurrentLevel();
	orderCameraTop();
}

void Game::loadNextLevel() {
	if (++currentLvl < maxGameLvl) {
		currentTheme++;
		changeTheme();
		prepareLevel();
	}
	else {
		wonGame = true;
		PlaySoundGame(Win);
	}
	Deactivate();
}
//2 3 -> 6 7
//1 4 -> 5 8
void Game::genSkyCubeHelper(vec3 a1, vec3 a2, vec3 a3, vec3 a4,
						vec3 a5, vec3 a6, vec3 a7, vec3 a8) {
	addShape(leveGenerator::create_square(a2, a3, a7, a6),
		-1, TRIANGLES, 12, 6);//top
	addShape(leveGenerator::create_square(a2, a3, a4, a1),
		-1, TRIANGLES, 13, 6);//front
	addShape(leveGenerator::create_square(a2, a6, a5, a1),
		-1, TRIANGLES, 14, 6);//left
	addShape(leveGenerator::create_square(a3, a7, a8, a4),
		-1, TRIANGLES, 15, 6);//right
	addShape(leveGenerator::create_square(a6, a7, a8, a5),
		-1, TRIANGLES, 16, 6);//back
}

inline void Game::genSkyHelper(float xl, float xh, float yl, float yh, float zl, float zh) {
	genSkyCubeHelper(vec3(xl, yh, zl), vec3(xl, yh, zh),
					 vec3(xh, yh, zh), vec3(xh, yh, zl),
					 vec3(xl, yl, zl), vec3(xl, yl, zh),
					 vec3(xh, yl, zh), vec3(xh, yl, zl));
}

const float epsilonSkyGen = -20.f;
const int amplify = 4;
void Game::genSky(float widthOfMap) {
	float low  = -amplify * widthOfMap - epsilonSkyGen;
	float high = (1 + amplify) * widthOfMap + epsilonSkyGen;
	genSkyHelper(low, high, low, high, low, high);
}
void Game::setupCurrentLevel() {
	printf("parsing file\n");

	struct objMap map = lGen->getLevel(currentLvl);
	genSky(glm::sqrt(float(map.levelGround->size()))*allscale);

	fruitCounter = 0;
	fruitsVec.clear();
	printf("loading level:%d walls:%d stairs:%d\n", currentLvl, map.walls->size(), map.stairs->size());

	if (map.levelGround != nullptr) {
		for (modelWrapper &obj : *map.levelGround) {
			addShape(obj.model, -1, TRIANGLES, themes->getTex(0), 4);
			shapes.back()->myTranslate(vec3(obj.x, obj.y, obj.z), 0);
		}
		for (modelWrapper &obj : *map.walls) {
			addShape(obj.model, -1, TRIANGLES, themes->getTex(1), 4);
			shapes.back()->myTranslate(vec3(obj.x, obj.y, obj.z), 0);
			IT->addObj(obj.x, obj.y, obj.level, shapes.back(),
				WallF, MeshConstructor::getlastInitMeshPositions());
		}
		for (modelWrapper &obj : *map.stairs) {
			addShape(obj.model, -1, TRIANGLES, themes->getTex(0), 4);
			shapes.back()->myTranslate(vec3(obj.x, obj.y, obj.z), 0);
		}
		for (modelWrapper &obj : *map.stairsWalls) {
			addShape(obj.model, -1, TRIANGLES, themes->getTex(0), 4);
			shapes.back()->myTranslate(vec3(obj.x, obj.y, obj.z), 0);
			shapes.back()->Hide();
			IT->addObj(obj.x, obj.y, obj.level, shapes.back(),
				StairF, MeshConstructor::getlastInitMeshPositions());
		}
		for (modelWrapper &obj : *map.fallWalls) {
			addShape(obj.model, -1, TRIANGLES, themes->getTex(0), 4);
			shapes.back()->myTranslate(vec3(obj.x, obj.y, obj.z), 0);
			shapes.back()->Hide();
			IT->addObj(obj.x, obj.y, obj.level, shapes.back(),
				FallWallF, MeshConstructor::getlastInitMeshPositions());
		}
		for (objLocation &obj : *map.specialObj)
			specialObjHandle(obj);
	}
	else
		printf("level did not been loaded!");
	setFruitMotion();
	isLoading = false;
}

void Game::setupEnvironment() {
	isLoading = true;
	setupCurrentLevel();
	//after adding snake we should order the camera
	setUpCamera();
	configSound();
	pickedShape = -1;
	printf("Game ready\n");
}

void Game::Init()
{
	loadThemes();
	setupEnvironment();
	//addCubes();
	ReadPixel();
}

void finUpdate(Shader *s, const int shaderIndx, const int pickedShape) {
	int r = ((pickedShape + 1) & 0x000000FF) >> 0;
	int g = ((pickedShape + 1) & 0x0000FF00) >> 8;
	int b = ((pickedShape + 1) & 0x00FF0000) >> 16;
	s->SetUniform4f("lightDirection", 0.0f, 0.0f, -1.0f, 0.0f);
	if (shaderIndx == 0)
		s->SetUniform4f("lightColor", r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	else
		s->SetUniform4f("lightColor", 1.0f, 1.0f, 1.0f, 1.0f);
	s->Unbind();
}


void Game::UpdateLinear(const glm::mat4 &lastMVP, const glm::mat4 &MVP, const glm::mat4 &nextMVP, const glm::mat4 &Normal, const int shaderIndx) {
	Shader *s = shaders[shaderIndx];
	s->Bind();
	s->SetUniformMat4f("lastMVP", lastMVP);
	s->SetUniformMat4f("MVP", MVP);
	s->SetUniformMat4f("nextMVP", nextMVP);
	s->SetUniformMat4f("Normal", Normal);
	finUpdate(s, shaderIndx, pickedShape);
}

void Game::Update(const glm::vec4 &camdir, glm::mat4 &MVP,const glm::mat4 &Normal,const int shaderIndx)
{
	Shader *s = shaders[shaderIndx];
	s->Bind();
	s->SetUniformMat4f("MVP", MVP);
	s->SetUniformMat4f("Normal", Normal);
	//printVec(camdir);
	s->SetUniform4f("Camdir", camdir.x, camdir.y, camdir.z, 0.f);
	finUpdate(s, shaderIndx, pickedShape);
}

void Game::WhenRotate() {}
void Game::WhenTranslate() {}

//speed also depends on user frame rate
int arrowKeyPL = 0;
void Game::setSnakeNodesAngles() 
{
	pickedShape = snakeNodesShapesStart;

	for (int i = 0; i < snakeLength - 1; i++) {//the last node(head) is turned only by the user
		motionTracker mT = sMT->getAngleAndAxis(i);
		float angle = mT.angleTurn;
		if (angle != 0) {
			vec3 axis = mT.rotationAxis;
			shapes[pickedShape]->myRotate(angle, axis, 4);
			pickedShape++;
			shapes[pickedShape]->myRotate(-angle, axis, 4);		
		}
		else
			pickedShape++;
	}
}

void Game::Debug() {
	Deactivate();
	IT->printDSDebug();
	//sMT->printDS();
}

const int maxFmotion = 20;
float angleFmotion = 8.f;
void Game::fruitMotion() {
	fruitMotionCounter += fruitMotionDir;
	if (fruitMotionCounter == maxFmotion) {
		fruitMotionDir = -1;
		motionJumps = -motionJumps;
	}
	else if (fruitMotionCounter == 0) {
		fruitMotionDir = 1;
		motionJumps = -motionJumps;
	}
	for (int i = 0; i < (signed) fruitsVec.size(); i++) {
		if (fruitsVec[i]->Is2Render()) {
			fruitsVec[i]->myTranslate(motionJumps, 1);
			fruitsVec[i]->myRotate(angleFmotion, zAx, zAxis1);
		}
	}
}

const int waitForBlink = 40;
const int waitForUnBlink = 4;
const int waitForTongue = 35;
const int waitForUnTongue = 5;
int curWaitForBlink = 0;
int curWaitForTongue = 0;
void Game::snakeFaceMotion() {
	if (curWaitForBlink == 0) {
		curWaitForBlink = waitForBlink + waitForUnBlink;
		shapes[snakeNodesShapesEnd + 1]->SetTexture(themes->getTex(4));
		shapes[snakeNodesShapesEnd + 2]->SetTexture(themes->getTex(4));
	}
	else if (curWaitForBlink == waitForBlink) {
		shapes[snakeNodesShapesEnd + 1]->SetTexture(0);
		shapes[snakeNodesShapesEnd + 2]->SetTexture(0);
	}
	if (curWaitForTongue == 0) {
		curWaitForTongue = waitForTongue + waitForUnTongue;
		shapes[snakeTongue]->myTranslate(vec3(0, 10, 0), 1);
	}
	else if (curWaitForTongue == waitForTongue) {
		shapes[snakeTongue]->myTranslate(vec3(0, -10, 0), 1);
	}
	curWaitForTongue--;
	curWaitForBlink--;

}

void Game::Motion()
{
	int savePicked = pickedShape;
	if (isActive)
	{	
		vec3 temp;
		if (superSpeedTicks > 0) {
			temp = superSpeed*tailDirection;
			superSpeedTicks--;
		}
		else if (rotRecently) {
			rotRecently = false;
			temp = slowSpeed*tailDirection;
		}
		else
			temp = speed*tailDirection;
		shapeTransformation(snakeNodesShapesStart, GlobalTranslate, temp);
		
		updateSnakePosition();
		setSnakeNodesAngles();
		updateCam();

		stairIntersCoolDown--;
		if (onStair) {
			IT->isIntersectSnakeHead(headTransMAT, headCurLocation.x, headCurLocation.y, snakeLevel - 1);
			IT->isIntersectSnakeHead(headTransMAT, headCurLocation.x, headCurLocation.y, snakeLevel);
			IT->isIntersectSnakeHead(headTransMAT, headCurLocation.x, headCurLocation.y, snakeLevel + 1);
		}
		else
			IT->isIntersectSnakeHead(headTransMAT, headCurLocation.x, headCurLocation.y, snakeLevel);
	}

	fruitMotion();
	snakeFaceMotion();

	pickedShape = savePicked;
}

float anglePL = 9.f;
void Game::changeCameraMode() {
	Deactivate();
	switchCamMode();
	Activate();
}

mat4 rotPl = glm::rotate(anglePL, zAx);
mat4 rotNPl = glm::rotate(-anglePL, zAx);
void Game::changeDirPInput(bool dir){
	Deactivate();

	rotRecently = true;
	int sign = (dir ? -1 : 1);
	arrowKeyPL += sign;
	sMT->add(zAx, sign*anglePL);
	shapes[snakeNodesShapesEnd]->myRotate(sign*anglePL, zAx, zAxis1);

	Activate();
}

void Game::playerInput(bool dir) {
	changeDirPInput(dir);
}

/*IMGUI HELPERS*/
void Game::switchSoundEnable() {
	if (soundEnable)
		PlaySoundGame(Hiss);//to stop the current sound in game
	soundEnable = !soundEnable;
}

//getters for imgui
bool Game::getSoundVar()		{	return soundEnable; }
int Game::getCurrentTheme()		{	return currentTheme; }
int Game::getCurrentLevel()		{	return currentLvl; }
int Game::getTotalLevelCount()	{	return lGen->size(); }
int Game::getCurrentFruitCount(){	return fruitsVec.size(); }
int Game::getTotalFruitCount()	{	return fruitsVec.size() - fruitCounter;}