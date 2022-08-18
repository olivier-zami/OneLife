//
// Created by olivier on 05/05/2022.
//

#include "Preview.h"

#include "../../../third_party/openLife/src/procedure/diagram/voronoi/FortuneAlgorithm.h"
#include "../../../third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/Node.hpp"


void initEdgePointsVis(beachline::HalfEdgePtr h, std::vector<double> &x, std::vector<double> &y,
					   const std::vector<Point2D> &points) {

	if (h->vertex != nullptr && h->twin->vertex != nullptr) {

		x[0] = h->vertex->point.x;
		x[1] = h->twin->vertex->point.x;

		y[0] = h->vertex->point.y;
		y[1] = h->twin->vertex->point.y;

	} else if (h->vertex != nullptr) {

		x[0] = h->vertex->point.x;
		y[0] = h->vertex->point.y;

		Point2D norm = (points[h->l_index] - points[h->r_index]).normalized().getRotated90CCW();
		x[1] = x[0] + norm.x * 1000;
		y[1] = y[0] + norm.y * 1000;

	} else if (h->twin->vertex != nullptr) {

		x[0] = h->twin->vertex->point.x;
		y[0] = h->twin->vertex->point.y;

		Point2D norm = (points[h->twin->l_index] - points[h->twin->r_index]).normalized().getRotated90CCW();
		x[1] = x[0] + norm.x * 1000;
		y[1] = y[0] + norm.y * 1000;

	} else {

		Point2D p1 = points[h->l_index], p2 = points[h->r_index];

		Point2D norm = (p1 - p2).normalized().getRotated90CCW();
		Point2D c = 0.5 * (p1 + p2);

		x[0] = c.x + norm.x * 1000;
		x[1] = c.x - norm.x * 1000;

		y[0] = c.y + norm.y * 1000;
		y[1] = c.y - norm.y * 1000;
	}
}

/**********************************************************************************************************************/

OneLife::mapManager::window::Preview::Preview(OneLife::mapManager::window::preview::Settings settings)
{
	this->showWindow = true;
	if(settings.renderer) this->renderer = settings.renderer;
	this->modified = false;
	this->content.map.dimension = {1000, 500};//settings.content.map.dimension;
	this->content.map.zoneCenter = new std::vector<Point2D>();;

	//!
	//int step = 125;
	int step = 75;
	for(int y=step; y<this->content.map.dimension.height; y+=step)
	{
		for(int x=step; x<this->content.map.dimension.width; x+=step)
		{
			this->content.map.zoneCenter->push_back(Point2D(x+(rand()%((step)-1))-(step/2), y+(rand()%((step)-1))-(step/2)));
		}
	}

	// Construct Voronoi diagram
	openLife::procedure::diagram::voronoi::VoronoiDiagram voronoiDiagram;
	voronoiDiagram.dimension.width = this->content.map.dimension.width;
	voronoiDiagram.dimension.height = this->content.map.dimension.height;
	openLife::procedure::diagram::voronoi::FortuneAlgorithm::generateDiagram(&voronoiDiagram, *(this->content.map.zoneCenter));

	//!
	OneLife::mapManager::window::widget::mapBuildingScreen::Settings mapBuildingScreenSettings;
	mapBuildingScreenSettings.renderer = this->renderer;
	mapBuildingScreenSettings.content.map.dimension = {this->content.map.dimension.width, this->content.map.dimension.height};
	mapBuildingScreenSettings.sharedData.map.zoneCenter = this->content.map.zoneCenter;
	this->mapBuildingScreen = new OneLife::mapManager::window::widget::MapBuildingScreen(mapBuildingScreenSettings);
	this->modified = true;

	for(size_t i=0; i<voronoiDiagram.edge->size(); i++)
	{
		if(voronoiDiagram.edge->at(i)->end[1] != nullptr)
		{
			this->mapBuildingScreen->tmpDrawLine(
					voronoiDiagram.edge->at(i)->end[0]->point.x,
					voronoiDiagram.edge->at(i)->end[0]->point.y,
					voronoiDiagram.edge->at(i)->end[1]->point.x,
					voronoiDiagram.edge->at(i)->end[1]->point.y);
		}
		else
		{
			this->mapBuildingScreen->drawSingleEdgeEnd(voronoiDiagram.edge->at(i)->end[0]->point);
			/*
			this->mapBuildingScreen->tmpDrawLine1(
					voronoiDiagram.edge->at(i)->end[0]->point.x,
					voronoiDiagram.edge->at(i)->end[0]->point.y,
					0,
					0);
			*/
		}
	}

	for(size_t i=0; i<voronoiDiagram.debug.point.size(); i++)
	{
		this->mapBuildingScreen->drawSmallCircle(voronoiDiagram.debug.point.at(i));
	}

	for(size_t i=0; i<voronoiDiagram.debug.point1.size(); i++)
	{
		this->mapBuildingScreen->drawSmallCircle1(voronoiDiagram.debug.point1.at(i));
	}
}

OneLife::mapManager::window::Preview::~Preview(){}

void OneLife::mapManager::window::Preview::render()
{
	if(this->showWindow)
	{
		if(this->modified)
		{
			this->mapBuildingScreen->showDotField();
			this->modified = false;
		}

		/*
		if(!this->previewSurface) this->createSurface();
		if(!this->previewTexture) this->createStreamingFrame();
		this->animate();
		*/

		ImGui::Begin("Map Generation", &(this->showWindow));
		this->mapBuildingScreen->render();
		ImGui::End();
	}
}