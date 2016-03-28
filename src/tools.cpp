#include <fstream>
#include <cmath>

#include "tools.h"
#include "kalman.h"

#define PI 3.14159265

double Tools::calcDist(double llat1, double llong1, double llat2, double llong2)
{
	// http://gis-lab.info/qa/great-circles.html
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
	//cout << (int)dist << endl;
	return dist;
}

string Tools::getPrevDay() {
	time_t seconds = time(NULL);
	tm* timeinfo = localtime(&seconds);
	char buff[20];
	//strcpy(output," Date:  ");
	//strcat(output,asctime(timeinfo));
	//cout << output << endl;
	//DATE_TIME_FORMAT
	strftime(buff, 20, DATE_TIME_FORMAT, timeinfo);
	timeinfo->tm_mday--;
	timeinfo->tm_hour = 23;
	timeinfo->tm_min = 59;
	timeinfo->tm_sec = 59;
	//strftime(buff, 20, DATE_TIME_FORMAT, localtime(&time));
	strftime(buff, 20, DATE_TIME_FORMAT, timeinfo);
	string res = buff;
	//		cout << res << endl;
	return res;
	//		strcpy(output," Date:  ");
	//		strcat(output,asctime(timeinfo));
	//		cout << output << endl;
}

string Tools::getString(double dbl) {
	std::ostringstream strs;
	strs << dbl;
	return strs.str();
}

void Tools::getGpx(PointList points, const string fileName) {
	gpx::ReportCerr report;
	gpx::GPX *root = new gpx::GPX();
	root->add("xmlns", gpx::Node::ATTRIBUTE)->setValue("http://www.topografix.com/GPX/1/1"); // Some tools need this
	root->version().add(&report)->setValue("1.1");
	root->creator().add(&report)->setValue("gpxcsvtrk");
	gpx::TRK* trk = dynamic_cast<gpx::TRK*>(root->trks().add(&report));
	trk->name().add(&report)->setValue("TEST");
	gpx::TRKSeg *trkseg = dynamic_cast<gpx::TRKSeg*>(trk->trksegs().add(&report));
	for (PointList::iterator it=points.begin(); it != points.end(); ++it) {
		if(it->getLat() > 0 && it->getLon() > 0) {
			gpx::WPT* trkpt = dynamic_cast<gpx::WPT*>(trkseg->trkpts().add(&report));
			trkpt->lat().add(&report)->setValue(getString(it->getLat()));
			trkpt->lon().add(&report)->setValue(getString(it->getLon()));
		}
	}
	gpx::Writer writer;
	ofstream stream(fileName);
	if (stream.is_open()) {
		writer.write(stream, root, false);
		stream.close();
	}
	else {
		cerr << "File: " << "out.gpx" << " could not be created." << endl;
	}
}

