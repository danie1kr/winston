#pragma once
#include <array>
#include <vector>
#include <string>
#include "WinstonTypes.h"

namespace winston
{

	/*
		tracks: [[[x,y],[x,y]], [[x,y], [x,y]] ,
		turnouts : [
			id: edge.data("track"),
			connection: edge.data("connection"),
			line: [[from.x, from.y], [to.x, to.y]]] ,
		bounds : {
		min: {
			x: 999999,
			y : 999999
		},
		max : {
			x: -999999,
			y : -999999
		}
		}
		*/
	struct RailwayMicroLayout
	{
		struct Point
		{
			int32_t x;
			int32_t y;
		};
		using Track = std::vector<Point>;
		using Tracks = std::vector<Track>;

		struct Connection
		{
			std::string connection;
			std::array<Point, 2> p;
		};

		using Connections = std::vector<Connection>;
		struct Turnout
		{
			std::string id;
			Connections connections;
		};
		using Turnouts = std::vector<Turnout>;

		struct TurnoutConnection
		{
			std::string turnout;
			std::string connection;
		};

		struct Bounds
		{
			Point min = { INT32_MAX, INT32_MAX }, max = { INT32_MIN, INT32_MIN };
			
			Bounds(int32_t minX, int32_t minY, int32_t maxX, int32_t maxY) :
				min{ minX, minY }, max{ maxX, maxY }
			{

			}

			Bounds() :
				min{ INT32_MAX, INT32_MAX }, max{ INT32_MIN, INT32_MIN }
			{

			}

			void update(const Point& p)
			{
				if (p.x < min.x) min.x = p.x;
				if (p.y < min.y) min.y = p.y;
				if (p.x > max.x) max.x = p.x;
				if (p.y > max.y) max.y = p.y;
			}
		};

		Tracks tracks;
		Turnouts turnouts;

		Bounds bounds;

		Result scale(const Bounds& screen)
		{
			int boundsWidth = this->bounds.max.x - this->bounds.min.x;
			int boundsHeight = this->bounds.max.y - this->bounds.min.y;

			if (boundsWidth == 0 || boundsHeight == 0)
				return Result::InvalidParameter;

			int screenWidth = screen.max.x - screen.min.x;
			int screenHeight = screen.max.y - screen.min.y;

			for (auto& track : this->tracks)
				for (auto& p : track)
				{
					p.x = screen.min.x + ((p.x - bounds.min.x) * screenWidth) / boundsWidth;
					p.y = screen.min.y + ((p.y - bounds.min.y) * screenHeight) / boundsHeight;
				}

			for (auto& turnout : this->turnouts)
			{
				for (auto& connection : turnout.connections)
				{
					for (auto& p : connection.p)
					{
						p.x = screen.min.x + ((p.x - bounds.min.x) * screenWidth) / boundsWidth;
						p.y = screen.min.y + ((p.y - bounds.min.y) * screenHeight) / boundsHeight;
					}
				}
			}

			this->bounds = screen;
			return winston::Result::OK;
		}
	};
}