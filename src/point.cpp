#include "point.h"

string Point::convertTime(const time_t time) const {
	char buff[20];
	strftime(buff, 20, DATE_TIME_FORMAT, localtime(&time));
	string res = buff;
	return res;
}

time_t Point::convertTime(const string time) const {
	struct tm tm;
	strptime(time.c_str(), DATE_TIME_FORMAT, &tm);
	return mktime(&tm);
}
