#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <math.h>

using namespace std;

class Tools
{
public:
	static string convertTime(time_t time) {
		char buff[20];
		//time_t now = time(NULL);
		//strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&time));
		strftime(buff, 20, "%F %T", localtime(&time));
		string res = buff;
		return res;
	}

	static time_t convertTime(string time) {
		struct tm tm;
		strptime(time.c_str(), "%F %T", &tm);
		return mktime(&tm);
	}

	/// Расчет расстояния между двумя географическими координатами.
	// http://gis-lab.info/qa/great-circles.html
	static double calcDist(double llat1, double llong1, double llat2, double llong2)
	{
#define PI 3.14159265

		//rad - радиус сферы (Земли)
		int rad = 6372795;

		//в радианах
		double lat1 = llat1 * PI/180;
		double lat2 = llat2 * PI/180;
		double long1 = llong1 * PI/180;
		double long2 = llong2 * PI/180;

		//косинусы и синусы широт и разницы долгот
		double cl1 = cos(lat1);
		double cl2 = cos(lat2);
		double sl1 = sin(lat1);
		double sl2 = sin(lat2);
		double delta = long2 - long1;
		double cdelta = cos(delta);
		double sdelta = sin(delta);

		//вычисления длины большого круга
		double y = sqrt(pow(cl2 * sdelta, 2) + pow(cl1 * sl2 - sl1 * cl2 * cdelta, 2));
		double  x = sl1 * sl2 + cl1 * cl2 * cdelta;
		double  ad = atan2(y, x);
		double  dist = ad * rad;
		//cout << llat1 << " " << llong1 << " / " << llat2 << " " << llong2 << " Dist: " << (int)dist << endl;
		return dist;
	}

};
#endif // TOOLS_H
