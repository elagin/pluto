#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>

#include <gpx/Parser.h>
#include <gpx/GPX.h>
#include <gpx/Writer.h>
#include <gpx/ReportCerr.h>

#include "point.h"

using namespace std;

class Tools
{
public:
	/// Расчет расстояния между двумя географическими координатами.
	static double calcDist(double llat1, double llong1, double llat2, double llong2);

	static string getPrevDay();

	static string getString(double dbl);

	static void getGpx(PointList points, const string fileName);

	static void showTime(int x) {
		int s = x / 1000;
		int hour = s / 3600;
		int min = (s - hour * 3600) / 60;
		int sec = s - hour * 3600 - min * 60;
		double msec = x % 1000;
		printf (" %02d:%02d:%02d.%d (%d)\n", hour, min, sec, (int)msec, x);
	}
};
#endif // TOOLS_H
