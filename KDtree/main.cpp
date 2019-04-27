#define _USE_MATH_DEFINES
/*
*   main.cpp
*  
*   k-d Tree Project
*	author: Arvind Rao
*   license: GNU 
*
*/  

#include "kdtree.h"
#include <iostream>
#include <array>
#include <bezier1D.cpp>
#include <bezier2D.cpp>
#include <mesh.h>


const int _numpts = 10;
const int _dim = 3;


Bezier2D b2d = Bezier2D::Bezier2D(Bezier1D::Bezier1D(), 3);
IndexedModel model = b2d.Bezier2D::GetSurface(5, 5);
std::vector<glm::vec3> testpoints2 = model.positions;
Node::vecType testpoints[_numpts] =
	{
		Node::vecType(38.40924, 4.11543, 8.10499,1), 
		Node::vecType(6.50689, 1.3663, 3.43026,1), 
		Node::vecType(9.7614, 9.8382, 0.672512,1), 
		Node::vecType(0.113181, 2.22785, 3.46726,1), 
		Node::vecType(5.23381, 4.69416, 4.74723,1), 
		Node::vecType(9.74655, 0.191659, 1.2064,1), 
		Node::vecType(2.8546, 7.32662, 8.51895,1), 
		Node::vecType(6.21829, 0.779546, 1.82988,1), 
		Node::vecType(8.83612, 8.70544, 2.40537,1), 
		Node::vecType(6.50697, 2.70078, 1.93852,1) 
	};


void printVector(Node::vecType pt)
{
	
	    std::cout<< pt.x << ", "<< pt.y << ", "<< pt.z<<"\n";
	
}

int main(int argc, char ** argv)
{ 
   
	
	//make a list of vectors out of the testpoints array
	std::list<Node::vecType> point_list;
	//for(auto i = 0; i < _numpts; i++ )
	for (auto i = 0; i < testpoints2.size(); i++)
	{	
		point_list.push_back(Bezier1D::v3to4(testpoints2[i]));
	}
	
	Kdtree kd;
	kd.makeTree(point_list);
	kd.printTree(kd.getRoot());
 	
	std::cout<<"\n\n";
	int a;	std::cin>>a;
}