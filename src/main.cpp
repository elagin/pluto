#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <list>
#include <ctime>

#include <soci/soci.h>
#include <soci-postgresql.h>
#include <soci/odbc/soci-odbc.h>

using namespace std;
using namespace soci;

struct Track {
	int blockId;
	double lat;
	double lon;
	string when;
	//time_t when;
};

typedef std::list<Track> TrackList;

static string SOURCE_DB = "DSN=***;UID=***;PWD=***;database=***";
static string GEOSERVER_DB = "dbname=*** user=*** password=*** host=***";

static string SRS = "4326";

bool lastUpdate(int blockId, string date)
{
	cout << "-= lastUpdate =-" << endl;
	try{
		session sql(postgresql, GEOSERVER_DB);
		string update = "UPDATE last_update SET date = :date WHERE block_id = :blockid";
		statement stUpdate = (sql.prepare << update, use(date, "date"), use(blockId, "blockid"));
		stUpdate.execute(true);
		if(stUpdate.get_affected_rows() == 0 ) {
			string insert = "INSERT INTO last_update(date, block_id) values(:date, :blockid)";
			statement stInsert = (sql.prepare << insert, use(date, "date"), use(blockId, "blockid"));
			stInsert.execute(true);
			return stInsert.get_affected_rows() > 0;
		}
	} catch (exception& e) {
		cout << "lastUpdate exception: " << e.what();
	}
	return false;
}

bool toLines(int blockId, TrackList& trackList)
{
	cout << "-= toLines =-" << endl;
	try{
		session sql(postgresql, GEOSERVER_DB);
		std::stringstream ss;
		std::stringstream lines;
		ss << "ST_GeomFromText('LINESTRING(";
		for (TrackList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
			if(lines.str().length() > 0) //Todo: Optimize me
				lines << ", ";
			lines << it->lon;
			lines << " ";
			lines << it->lat;
		}
		ss << lines.str();
		ss << ")', 4326)";
		Track last = trackList.back();
		string sqlLine = "INSERT INTO lines(block_id, date, shape) values(:blockid, :date ," + ss.str() + ")";
		statement st = (sql.prepare << sqlLine, use(blockId, "blockid"), use(last.when, "date"));
		return lastUpdate(blockId, last.when);
	} catch (exception& e) {
		cout << "toLines exception: " << e.what();
	}
	return false;
}

void toPoints(int blockId, TrackList &trackList)
{
	cout << "-= toPoints =-" << endl;
	try{
		session sql(postgresql, GEOSERVER_DB);
		for (TrackList::iterator it=trackList.begin(); it != trackList.end(); ++it) {
			std::stringstream ss;
			ss << "ST_GeomFromText('POINT(";
			ss << it->lon;
			ss << " ";
			ss << it->lat;
			ss << ")', " + SRS + ")";
			string sqlLine = "INSERT INTO points(block_id, date, shape) values(:blockid, :date ," + ss.str() + ")";
			statement st = (sql.prepare << sqlLine, use(blockId, "blockid"), use(it->when, "date"));
			st.execute(true);
		}
	} catch (exception& e) {
		cout << "toPoints exception: " << e.what();
	}
}

time_t convertTime(string time) {
	struct tm tm;
	strptime(time.c_str(), "%F %T", &tm);
	return mktime(&tm);
}

string getTail()
{
	cout << "-= getTail =-" << endl;
	int block_id = 187156;
	string res;
	string sqlReq = "SELECT top 10 lat, lon, received_date FROM journal_mon_201404142011.mld_message WHERE block_id = :block ORDER BY received_date";

	try{
		session sql(odbc, SOURCE_DB);
		TrackList trackList;
		row rowData;
		statement st = (sql.prepare << sqlReq, use(block_id), into(rowData));
		if( st.execute(true) ) {
			Track track;
			track.blockId = block_id;
			track.lat = rowData.get<double>(0);
			track.lon = rowData.get<double>(1);
			//track.when = convertTime(rowData.get<std::string>(2));
			track.when = rowData.get<std::string>(2);
			trackList.push_back(track);
			while (st.fetch()){
				Track track;
				track.blockId = block_id;
				track.lat = rowData.get<double>(0);
				track.lon = rowData.get<double>(1);
				//track.when = convertTime(rowData.get<std::string>(2));
				track.when = rowData.get<std::string>(2);
				trackList.push_back(track);
			}
			cout << "----== Size: " << trackList.size() << endl;
			toPoints(block_id, trackList);
			toLines(block_id, trackList);
			cout << res << endl;
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
		return e.what();
	}
	return res;
}


int main(int argc, char const* argv[]) {
	getTail();
	return (EXIT_SUCCESS);
}