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

#include <libconfig.h++>

#include "point.h"
#include "tools.h"
#include "kalman.h"

using namespace std;
using namespace soci;
using namespace std::chrono;
using namespace libconfig;

static const string TOP_LINES = "100000"; // SELECT TOP
static const string SRS = "4326";
static const string TABLE_POSTFIX = "_test";

static const int MAX_POINTS_DIST = 1000; //Максимальное расстояние между точками для отреза линии.
static const int RESERVE = 100000;		//Резервирование для вектора

static string source_db;
static string geoserver_db;

void checkDist(PointList &trackList) {
	cout << "checkDist" << endl;
	Point prev;
	for (PointList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
		if(it != trackList.begin()) {
			double dist = Tools::calcDist(prev.getLat(), prev.getLon(), it->getLat(), it->getLon());
			if(dist >= MAX_POINTS_DIST) {
//				cout << "Zero 1" << endl;
//				prev.toString();
//				it->toString();
//				cout << "Zero 2" << endl;
			}
			//cout << (int)dist << endl;
		}
		prev = *it;
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
		session sql(postgresql, geoserver_db);
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
		cout << "lastUpdate exception: " << e.what() << endl;
	}
	return false;
}

bool toLines(int blockId, PointList& trackList, string lastWhen) {
	//cout << "-= toLines =-" << endl;
	if(trackList.size() <= 1) {
		//Для линии требуется хотя бы две точки
		cout << "Enough points to create a line: " << blockId << endl;
		return true;
	}
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(postgresql, geoserver_db);
		stringstream ss;
		stringstream lines;
		ss << "ST_GeomFromText('LINESTRING(";
		for (PointList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
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
		cout << "toLines exception: " << e.what() << endl;
	}
	return false;
}

list<string> getLines(PointList& trackList) {
	list<string> lines;
	stringstream line;
	Point prev;
	int points = 0;
	for (PointList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
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

bool toCutLines(int blockId, PointList& trackList, string lastWhen) {
	//cout << "-= toLines =-" << endl;
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(postgresql, geoserver_db);
		string sqlLine = "INSERT INTO lines" + TABLE_POSTFIX + "(block_id, date, shape) VALUES ";
		list<string> lines = getLines(trackList);
		stringstream values;
		for (list<string>::iterator it=lines.begin(); it != lines.end(); ++it) {
			//cout << "Line: " << it->c_str() << endl;
			stringstream value;
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
		cout << "toLines exception: " << e.what() << endl;
	}
	return false;
}

bool toPoints(int blockId, PointList &trackList) {
	//cout << "-= toPoints =-" << endl;
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(postgresql, geoserver_db);
		string insertSql = "INSERT INTO points" + TABLE_POSTFIX + "(block_id, date, shape) VALUES ";
		stringstream values;
		for (PointList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
			stringstream ss;
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
		//cout << "toPoints affected_rows: " << st.get_affected_rows() << endl;
		high_resolution_clock::time_point endTime = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>( endTime - startTime ).count();
		cout << "toPoints execution-time is: ";
		showTime(duration);
		return st.get_affected_rows() > 0;
	} catch (exception& e) {
		cout << "toPoints exception: " << e.what() << endl;
	}
	return false;
}

string getLastData(int block_id) {
	cout << "-= getLastData =-" << endl;
	try{
		session sql(postgresql, geoserver_db);
		string select = "SELECT date FROM last_update" + TABLE_POSTFIX + " WHERE block_id = :blockid";
		string date;
		statement st = (sql.prepare << select, use(block_id, "blockid"), into(date));
		if( st.execute(true) ) {
			cout << "getLastData: old date is: " << date << endl;
			return date;
		} else {
			//cout << "getLastData: No data for blockid: " << block_id << endl;
		}
	} catch (exception& e) {
		cout << "getLastData exception: " << e.what();
	}
	return "";
}

struct LastUpdate{
	int blockId;
	string date;
};

typedef list<LastUpdate> UpdateList;

UpdateList getLastDataList() {
	const int totalBlocks = 1200;
	cout << "-= getLastData =-" << endl;
	UpdateList updateList;
	try{
		vector<string> dates(totalBlocks);
		vector<int> blocks(totalBlocks);
		vector<indicator> inds(totalBlocks);
		session sql(postgresql, geoserver_db);
		string select = "SELECT date, block_id FROM last_update" + TABLE_POSTFIX;
		cout << select << endl;
		sql << select, into(dates, inds), into(blocks);
		for(int i = 0; i < blocks.size(); i++) {
			LastUpdate lastUpdate;
			if(inds[i] != soci::i_null)
				lastUpdate.date = dates[i];
			lastUpdate.blockId = blocks[i];
			updateList.push_back(lastUpdate);
		}
	} catch (exception& e) {
		cout << "getLastData exception: " << e.what() << endl;
	}
	cout << "getLastData count: " << updateList.size() << endl;
	return updateList;
}

void getTail(int block_id, string date) {
	string prevDay = Tools::getPrevDay();
	//cout << "-= getTail block_id: " << block_id << " date: " << date << endl;
	string sqlReq = "SELECT top " + TOP_LINES + " lat, lon, received_date, id FROM journal_mon_201502140953.mld_message WITH(NOLOCK)";
	stringstream where;
	where << "WHERE block_id = :blockid AND received_date < '" << prevDay << "'";
	if(!date.empty())
		where << "AND received_date > '" << date << "'";
	where << " AND lat IS NOT NULL AND lon IS NOT NULL ORDER BY received_date";
	try{
		high_resolution_clock::time_point startTime = high_resolution_clock::now();
		session sql(odbc, source_db);
		PointList trackList;
		trackList.reserve(RESERVE);
		row rowData;
		string lastWhen = "";
		high_resolution_clock::time_point startRequestTime = high_resolution_clock::now();
		statement st = (sql.prepare << sqlReq << where.str(), use(block_id, "blockid"), into(rowData));
		//cout << sqlReq << where.str() << endl;
		int totalRows = 0;
		//Point prevPoint;
		if( st.execute(true) ) {
			high_resolution_clock::time_point stopRequestTime = high_resolution_clock::now();
			auto mssqlDuration = duration_cast<milliseconds>( stopRequestTime - startRequestTime ).count();
			cout << "MSSQL execution-time is: ";
			showTime(mssqlDuration);
			high_resolution_clock::time_point startPrepareTime = high_resolution_clock::now();
			lastWhen = rowData.get<std::string>(2);
			//Point firstPoint(rowData.get<double>(0), rowData.get<double>(1), lastWhen);
			Point prevPoint(rowData.get<double>(0), rowData.get<double>(1), lastWhen);
			//trackList.push_back(firstPoint);
			//prevPoint = firstPoint;
			totalRows++;
			while (st.fetch()){
				lastWhen = rowData.get<std::string>(2);
				Point point(rowData.get<double>(0), rowData.get<double>(1), lastWhen);
				// Если новая координата
				if(point.getLat() != prevPoint.getLat() || point.getLon() != prevPoint.getLon()){
					double dist = Tools::calcDist(prevPoint.getLat(), prevPoint.getLon(), point.getLat(), point.getLon());
					// и расстояние меньше допустимого
					if(dist <= MAX_POINTS_DIST) {
						// добавляем в список
						trackList.push_back(point);
						//Только в этом случае, иначе на следующем шаге будем проверять относительно кривой точки.
						prevPoint = point;
					}
				} else {
					prevPoint = point;
				}
				//prevPoint = point;
				totalRows++;
			}
			high_resolution_clock::time_point stopPrepareTime = high_resolution_clock::now();
			auto prepareDuration = duration_cast<milliseconds>( stopPrepareTime - startPrepareTime ).count();
			cout << "prepareDuration execution-time is: ";
			showTime(prepareDuration);
			cout << "BlockID: " << block_id << " points to insert: " << trackList.size() << " diference: " << (totalRows - trackList.size()) << endl;
			//checkDist(trackList);
			//string gpsName = "out_" + std::to_string(block_id) + ".gpx";
			if(trackList.size() > 0)
//				Tools::getGpx(trackList, gpsName);
				if(toPoints(block_id, trackList))
				//if(toCutLines(block_id, trackList, lastWhen))
					if(trackList.size() > 1)
						if(toLines(block_id, trackList, lastWhen))
							toLastUpdate(block_id, lastWhen);
			high_resolution_clock::time_point endTime = high_resolution_clock::now();
			auto duration = duration_cast<milliseconds>( endTime - startTime ).count();
			cout << "getTail execution-time is: ";
			showTime(duration);
			cout << "Next date is: " << lastWhen << endl;
		} else {
			//cout << "No data for blockid: " << block_id << endl;
		}
	} catch (exception& e) {
		cout << "getTail exception: " << e.what() << endl;
	}
}

void currentTime() {
	time_t rawtime;
	struct tm* timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	printf ( "Current local time and date: %s", asctime (timeinfo) );
}

bool loadCfg() {
	const char* configFileName = "pluto.cfg";
	Config config;
	try {
		config.readFile(configFileName);
	}
	catch(const FileIOException &fioex) {
		std::cerr << "I/O error while reading config file: " << configFileName << std::endl;
		return false;
	}
	catch(const ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
		return false;
	}

	try {
		source_db = config.lookup("application.source_db").c_str();
		geoserver_db = config.lookup("application.geoserver_db").c_str();
		return true;
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Invalid setting in configuration file." << endl;
		return false;
	}
}

int main(int argc, char const* argv[]) {
	if(loadCfg()) {
		cout << "Work with '" << TABLE_POSTFIX << "'' prefix =========================================" << endl;
		currentTime();
		for (int i = 0; i < 1;i++) {
			UpdateList list = getLastDataList();
			for (UpdateList::iterator it=list.begin(); it != list.end(); ++it) {
				getTail(it->blockId, it->date);
			}
			cout << "==============================" << endl;
		}
		currentTime();
	}
	return (EXIT_SUCCESS);
}
