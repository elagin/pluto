#ifndef POINT_H
#define POINT_H

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

static const char* DATE_TIME_FORMAT = "%F %T";

class Point
{
private:
	double lat = 0;
	double lon = 0;
	time_t when;

private:
	string convertTime (const time_t time) const;
	time_t convertTime (const string time) const;

public:
	Point() {}
	Point(double lat, double lon, string when)
		: lat(lat), lon(lon){
		this->when = convertTime(when);
	}

	double getLat() const {
		return lat;
	}

	double getLon() const {
		return lon;
	}

	string getWhen() const {
		return convertTime(when);
	}

	bool operator < (const Point &point) const {
		return (this->lat < point.lat || this->lon < point.lon) && this->when < point.when; //убирает лишние, но оставляет только меньшее время.
	}

	void toString() const {
		cout << lat << " " << lon << " " << convertTime(when) << endl;
	}
};

typedef vector<Point> PointList;

#endif // POINT_H
