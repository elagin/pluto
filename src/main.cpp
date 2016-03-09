#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <list>
#include <ctime>
#include <chrono>
#include <set>

#include <soci/soci.h>
#include <soci-postgresql.h>
#include <soci/odbc/soci-odbc.h>

#include "tools.h"

using namespace std;
using namespace soci;
using namespace std::chrono;

static const string SOURCE_DB = "DSN=***;UID=***;PWD=***;database=***";
static const string GEOSERVER_DB = "dbname=*** user=*** password=*** host=***";
static const string TOP_LINES = "100"; // SELECT TOP
static const string SRS = "4326";
static const string TABLE_POSTFIX = "_200";

class Track {
private:
	int blockId = 0;
	double lat = 0;
	double lon = 0;
	time_t when;

public:
	Track() {}
	Track(int blockId, double lat, double lon, string when)
		: blockId(blockId), lat(lat), lon(lon){
		this->when = Tools::convertTime(when);
	}

	int getBlockId() const {
		return blockId;
	}

	double getLat() const {
		return lat;
	}
	double getLon() const {
		return lon;
	}

	string getWhen() const {
		return Tools::convertTime(this->when);
	}

	bool operator < (const Track &point) const {
		return (this->lat < point.lat || this->lon < point.lon) && this->when < point.when; //убирает лишние, но оставляет только меньшее время.
	}

	void toString() const {
		cout << blockId << " " << lat << " " << lon << " " << Tools::convertTime(when) << endl;
	}
};

typedef std::set<Track> TrackList;

static const int MAX_POINTS_DIST = 20000; //Максимальное расстояние между точками для отреза линии.

void checkDist(TrackList &trackList) {
	Track prev;
	for (TrackList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
		if(prev.getBlockId() == 0 ) {
			prev = *it;
		} else {
			double dist = Tools::calcDist(prev.getLat(), prev.getLon(), it->getLat(), it->getLon());
			prev = *it;
		}
	}
}

void showTime(int x) {
	int s = x / 1000;
	int hour = s / 3600;
	int min = (s - hour * 3600) / 60;
	int sec = s - hour * 3600 - min * 60;
	double msec = x % 1000;
	printf (" %02d:%02d:%02d.%d (%d)\n", hour, min, sec, (int)msec, x);
}

bool toLastUpdate(int blockId, string date) {
	cout << "-= lastUpdate =-" << endl;
	try{
		session sql(postgresql, GEOSERVER_DB);
		string update = "UPDATE last_update" + TABLE_POSTFIX + " SET date = :date WHERE block_id = :blockid";
		statement stUpdate = (sql.prepare << update, use(date, "date"), use(blockId, "blockid"));
		stUpdate.execute(true);
		if(stUpdate.get_affected_rows() == 0 ) {
			string insert = "INSERT INTO last_update" + TABLE_POSTFIX + "(date, block_id) values(:date, :blockid)";
			statement stInsert = (sql.prepare << insert, use(date, "date"), use(blockId, "blockid"));
			stInsert.execute(true);
			return stInsert.get_affected_rows() > 0;
		}
	} catch (exception& e) {
		cout << "lastUpdate exception: " << e.what();
	}
	return false;
}

bool toLines(int blockId, TrackList& trackList, string lastWhen) {
	cout << "-= toLines =-" << endl;
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(postgresql, GEOSERVER_DB);
		std::stringstream ss;
		std::stringstream lines;
		ss << "ST_GeomFromText('LINESTRING(";
		for (TrackList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
			if(lines.str().length() > 0) //Todo: Optimize me
				lines << ", ";
			lines << it->getLon();
			lines << " ";
			lines << it->getLat();
		}
		ss << lines.str();
		ss << ")', 4326)";

		string sqlLine = "INSERT INTO lines" + TABLE_POSTFIX + "(block_id, date, shape) values(:blockid, :date ," + ss.str() + ")";
		statement st = (sql.prepare << sqlLine, use(blockId, "blockid"), use(lastWhen, "date"));
		st.execute(true);
		high_resolution_clock::time_point endTime = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>( endTime - startTime ).count();
		cout << "toLines execution-time is: ";
		showTime(duration);
		return st.get_affected_rows() > 0;
	} catch (exception& e) {
		cout << "toLines exception: " << e.what();
	}
	return false;
}

list<string> getLines(TrackList& trackList) {
	list<string> lines;
	stringstream line;
	Track prev;
	int points = 0;
	for (TrackList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
		if(line.str().length() == 0 ) {
			// первая итерация
			line << it->getLon();
			line << " ";
			line << it->getLat();
			points++;
		} else {
			double dist = Tools::calcDist(prev.getLat(), prev.getLon(), it->getLat(), it->getLon());
			if(dist <= MAX_POINTS_DIST) {
				// продолжаем линию
				line << ", ";
				line << it->getLon();
				line << " ";
				line << it->getLat();
				points++;
			} else {
				//отрезаем линию
				//cout << "Points into line is: " << points << endl;
				if(points > 1)
					lines.push_back(line.str());
				line.str("");
				points = 0;
			}
		}
		prev = *it;
	}
	if(line.str().length() > 0) {
		//cout << "Points into line is: " << items << endl;
		lines.push_back(line.str());
	}
	cout << "Total lines: " << lines.size() << endl;
	return lines;
}

bool toCutLines(int blockId, TrackList& trackList, string lastWhen) {
	cout << "-= toLines =-" << endl;
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(postgresql, GEOSERVER_DB);
		string sqlLine = "INSERT INTO lines" + TABLE_POSTFIX + "(block_id, date, shape) VALUES ";
		list<string> lines = getLines(trackList);
		std::stringstream values;
		for (list<string>::iterator it=lines.begin(); it != lines.end(); ++it) {
			//cout << "Line: " << it->c_str() << endl;
			std::stringstream value;
			value << "( " << blockId << ", '" << lastWhen << "', " << "ST_GeomFromText('LINESTRING(" << it->c_str() << ")', 4326) )";
			//cout << "value: " << value.str() << endl;
			if(values.str().size() > 0)
				values << ", ";
			values << value.str();
		}
		//cout << sqlLine << values.str() << endl;
		statement st = (sql.prepare << sqlLine << values.str());
		st.execute(true);
		high_resolution_clock::time_point endTime = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>( endTime - startTime ).count();
		cout << "toLines execution-time is: ";
		showTime(duration);
		return st.get_affected_rows() > 0;
	} catch (exception& e) {
		cout << "toLines exception: " << e.what();
	}
	return false;
}

bool toPoints(int blockId, TrackList &trackList) {
	cout << "-= toPoints =-" << endl;
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(postgresql, GEOSERVER_DB);
		string insertSql = "INSERT INTO points" + TABLE_POSTFIX + "(block_id, date, shape) VALUES ";
		std::stringstream values;
		for (TrackList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
			std::stringstream ss;
			ss << "( " << blockId << ", '" << it->getWhen() << "', ";
			ss << "ST_GeomFromText('POINT(";
			ss << it->getLon();
			ss << " ";
			ss << it->getLat();
			ss << ")', " + SRS + ")";
			if(values.str().size() != 0) {
				values << ", ";
			}
			values << ss.str();
			values << " )";
//			string insertSql = "INSERT INTO points(block_id, date, shape) values(:blockid, :date ," + ss.str() + ")";
//			statement st = (sql.prepare << insertSql, use(blockId, "blockid"), use(it->when, "date"));
//			st.execute(true);
		}
		//cout << insertSql << values.str();
		statement st = (sql.prepare << insertSql << values.str());
		st.execute(true);
		cout << "toPoints affected_rows: " << st.get_affected_rows() << endl;
		high_resolution_clock::time_point endTime = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>( endTime - startTime ).count();
		cout << "toPoints execution-time is: ";
		showTime(duration);
		return st.get_affected_rows() > 0;
	} catch (exception& e) {
		cout << "toPoints exception: " << e.what();
	}
	return false;
}

string getLastData(int block_id) {
	cout << "-= getLastData =-" << endl;
	try{
		session sql(postgresql, GEOSERVER_DB);
		string select = "SELECT date FROM last_update" + TABLE_POSTFIX + " WHERE block_id = :blockid";
		string date;
		statement st = (sql.prepare << select, use(block_id, "blockid"), into(date));
		if( st.execute(true) ) {
			cout << "getLastData: old date is: " << date << endl;
			return date;
		} else {
			cout << "getLastData: No data for blockid: " << block_id << endl;
		}
	} catch (exception& e) {
		cout << "getLastData exception: " << e.what();
	}
	return "";
}

void getTail(int block_id, string date) {
	cout << "-= getTail block_id: " << block_id << " date: " << date << endl;
	string sqlReq = "SELECT top " + TOP_LINES + " lat, lon, received_date FROM journal_mon_201404142011.mld_message WHERE block_id = :blockid AND received_date > :date ORDER BY received_date";

	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(odbc, SOURCE_DB);
		TrackList trackList;
		row rowData;
		string lastWhen = "";
		high_resolution_clock::time_point startRequestTime = high_resolution_clock::now();
		statement st = (sql.prepare << sqlReq, use(block_id, "blockid"), use(date, "date"), into(rowData));
		int totalRows = 0;
		if( st.execute(true) ) {
			high_resolution_clock::time_point stopRequestTime = high_resolution_clock::now();
			auto mssqlDuration = duration_cast<milliseconds>( stopRequestTime - startRequestTime ).count();
			cout << "MSSQL execution-time is: ";
			showTime(mssqlDuration);

			high_resolution_clock::time_point startPrepareTime = high_resolution_clock::now();
			lastWhen = rowData.get<std::string>(2);
			Track track(block_id, rowData.get<double>(0), rowData.get<double>(1), lastWhen);
			trackList.insert(track);
			totalRows++;
			while (st.fetch()){
				lastWhen = rowData.get<std::string>(2);
				Track track(block_id, rowData.get<double>(0), rowData.get<double>(1), lastWhen);
				trackList.insert(track);
				totalRows++;
			}
			high_resolution_clock::time_point stopPrepareTime = high_resolution_clock::now();
			auto prepareDuration = duration_cast<milliseconds>( stopPrepareTime - startPrepareTime ).count();
			cout << "prepareDuration execution-time is: ";
			showTime(prepareDuration);
			cout << "Size to insert: " << trackList.size() << " diference: " << (totalRows - trackList.size()) << endl;
			//if(toPoints(block_id, trackList))
				if(toCutLines(block_id, trackList, lastWhen))
				//if(toLines(block_id, trackList, lastWhen))
					toLastUpdate(block_id, lastWhen);

			high_resolution_clock::time_point endTime = high_resolution_clock::now();
			auto duration = duration_cast<milliseconds>( endTime - startTime ).count();
			cout << "getTail execution-time is: ";
			showTime(duration);
			cout << "Next date is: " << lastWhen << endl;
		} else {
			cout << "No data for blockid: " << block_id << endl;
		}

		//INSERT INTO my_map (name, shape)  VALUES (  'Track5', ST_GeomFromText('LINESTRING(
		//0.0 0.0,
		//30.31667 59.95000,
		//37.61778 55.75583
		//)', 4326) );
	} catch (exception& e) {
		cout << e.what();
	}
}

int main(int argc, char const* argv[]) {
	int block_id = 187156;
	//block_id = 14930;
	string date = getLastData(block_id);
	getTail(block_id, date);
	return (EXIT_SUCCESS);
}
