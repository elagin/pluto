#ifndef KALMAN_H
#define KALMAN_H

#include "point.h"

class Kalman{

struct Res {
	double mean;
	double var;
};

protected:
	double motionSig;
	double measurementSig;

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

public:
	Kalman(double motionSig  = 1.0, double measurementSig = 10.0)
		: motionSig(motionSig), measurementSig(measurementSig) {}

	PointList run(PointList points) {
		PointList result;
		PointList::iterator itBegin=points.begin();
		Res resLng;
		resLng.mean = itBegin->getLon();
		resLng.var = 1.0;

		Res resLat;
		resLat.mean = itBegin->getLat();
		resLat.var = 1.0;
		for (PointList::iterator it=points.begin()++; it != points.end(); ++it) {
			resLng = update(resLng.mean, resLng.var, it->getLon(), measurementSig);
			resLng = predict(resLng.mean, resLng.var, 0, motionSig);
			double resultLng(resLng.mean);

			resLat = update(resLat.mean, resLat.var, it->getLat(), measurementSig);
			resLat = predict(resLat.mean, resLat.var, 0, motionSig);
			double resultLat(resLat.mean);
			Point resPoint(resultLat, resultLng, it->getWhen());
			result.push_back(resPoint);
			cout.precision(6);
			cout << fixed << it->getLat() - resultLat << endl;
		}
		return result;
	}
};

#endif // KALMAN_H

