#ifndef KALMAN_H
#define KALMAN_H

#include "point.h"

class kalman{

struct Res {
	double mean;
	double var;
};

protected:

	double motion_sig = 1.0;
	double measurement_sig = 10.0;

	Res update(double mean1, double var1, double mean2, double var2) {
		Res res;
		res.mean = (var2 * mean1 + var1 * mean2) / (var1 + var2);
		res.var = 1 / (1 / var1 + 1 / var2);
		return res;
	}

	Res predict(double mean1, double var1, double mean2, double var2) {
		Res res;
		res.mean = mean1 + mean2;
		res.var = var1 + var2;
		return res;
	}

//	def update(mean1, var1, mean2, var2):
//	    new_mean = (var2 * mean1 + var1 * mean2) / (var1 + var2)
//	    new_var = 1 / (1 / var1 + 1 / var2)
//	    return [new_mean, new_var]

//	def predict(mean1, var1, mean2, var2):
//	    new_mean = mean1 + mean2
//	    new_var = var1 + var2
//	    return [new_mean, new_var]

public:
	void run(PointList points) {
		double sigLng = 1.0;
		double sigLat = 1.0;

		PointList::iterator it=points.begin();

		Res resLng;
		resLng.mean = it->getLon();
		resLng.var = 1.0;

		Res resLat;
		resLat.mean = it->getLat();
		resLat.var = 1.0;



		for (PointList::iterator it=points.begin(); it != points.end(); ++it) {
			//stringstream ss;
			//ss << "( " << it->getWhen() << "', ";

//			Res resLng = update(lng, sigLng, longitudes[i], measurement_sig);
//			predict(lng, sigLng, 0, motion_sig, lng, sigLng)

//			for i in range(len(positions)):
//			    lng, sigLng = update(lng, sigLng, longitudes[i], measurement_sig)
//			    lng, sigLng = predict(lng, sigLng, 0, motion_sig)
//			    resultLng.append(lng)

//			    lat, sigLat = update(lat, sigLat, latitudes[i], measurement_sig)
//			    lat, sigLat = predict(lat, sigLat, 0, motion_sig)
//			    resultLat.append(lat)

		}
//		double lng = longitudes[0];
//		double lat = latitudes[0];
	}
};

#endif // KALMAN_H
