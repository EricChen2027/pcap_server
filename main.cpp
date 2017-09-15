#include <mysql/mysql.h>
#include <libconfig.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
using namespace std;

#include "jsoncpp/json.h"
#include "common.h"
#include "mycurl.h"

void task_processer(MEDIA_INFO media_info_item, char *file_path_split_prefix, MYSQL *pconn) {
	if ("" == file_path_split_prefix || "" == media_info_item.data_path || NULL == pconn) {
		return;
	}
	char command[128] = {'\0'};
	if (strcmp(media_info_item.format_name, "mpegts") == 0) {
		sprintf(command, "cat %s > %s", file_path_split_prefix, media_info_item.data_path);
		system(command);
	}else {
		//concat the stream data which is forced to format as a *.mp4
		sprintf(command, "ffmpeg -f concat -safe 0 -i file_list.txt -c copy -f mp4 %s", media_info_item.data_path);
		system(command);
	}
	// get the streams info
	sprintf(command, "ffprobe -show_streams -print_format json %s", media_info_item.data_path);
	FILE *fd = popen(command, "r");							
	if (fd) {
		char buffer[128] = {'\0'};
		char stream_data[4096] = {'\0'};
		int pos = 0;
		while(!feof(fd)) {
			if (fgets(buffer, 128, fd) == NULL) break;
			strncpy(stream_data + pos, buffer, strlen(buffer));
			pos += strlen(buffer);
		}
		char stream[4096] = {'\0'};
		strtotoken(stream_data, stream, 4096, " \r\n");
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(stream, root)) {
			if (root["streams"].isArray()) {
				for (int i = 0; i < root["streams"].size(); i++) {
					if (root["streams"][i]["codec_type"].asString().compare("video") == 0) {
						char video_codec_info[4096] = {'\0'};
						strtotoken((char*)(root["streams"][i].toStyledString().c_str()), media_info_item.video_codec_info, 4096, " \r\n");
						strcpy(media_info_item.codec_type, root["streams"][i]["codec_name"].asString().c_str());
						strcpy(media_info_item.video_bit_rate, root["streams"][i]["bit_rate"].asString().c_str());
					}
					if (root["streams"][i]["codec_type"].asString().compare("audio") == 0) {
						char audio_codec_info[4096] = {'\0'};
						strtotoken((char*)(root["streams"][i].toStyledString().c_str()), media_info_item.audio_codec_info, 4096, " \r\n");
						strcpy(media_info_item.duration, root["streams"][i]["duration"].asString().c_str());
						strcpy(media_info_item.audio_bit_rate, root["streams"][i]["bit_rate"].asString().c_str());
					}
				}
			}
		}
		pclose(fd);
	}
	//get format
	sprintf(command, "ffprobe -show_format -print_format json %s", media_info_item.data_path);
	fd = popen(command, "r");
	if (fd) {
		char buffer[128] = {'\0'};
	    char format_data[1024] = {'\0'};
	    int pos = 0;
	    while(!feof(fd)) {
	        if (fgets(buffer, 128, fd) == NULL) break;
	        strncpy(format_data + pos, buffer, strlen(buffer));
	        pos += strlen(buffer);
		}
		char format[1024] = {'\0'};
		strtotoken(format_data, format, 1024, " \r\n");
	    Json::Reader reader;
	    Json::Value root;
	    if (reader.parse(format, root)) {
			strcpy(media_info_item.bit_rate, root["format"]["bit_rate"].asString().c_str());
			if (media_info_item.video_bit_rate == "") {
	        	if (media_info_item.audio_bit_rate != "") {
   		     		int bitrate = atoi(root["format"]["bit_rate"].asString().c_str()) - atoi(media_info_item.audio_bit_rate);
					sprintf(media_info_item.video_bit_rate, "%d", bitrate);
	            }else {
					strcpy(media_info_item.video_bit_rate, root["format"]["bit_rate"].asString().c_str());
	            }
   		    }
		}
		pclose(fd);
	}

	//insert into the database
	time_t currTime = time(NULL);
	struct tm *localTime = localtime(&currTime);
	char time_format[20] = {'\0'};
   	sprintf(time_format, "%d-%02d-%02d %02d:%02d:%02d",
	 		localTime->tm_year + 1900,
			localTime->tm_mon + 1,
			localTime->tm_mday,
			localTime->tm_hour,
			localTime->tm_min,
			localTime->tm_sec);	
	char sql[10000] = {'\0'};
	sprintf(sql, "insert into competitor_ondemand_results(`site`, `platform`, `program_type`, `resolution`, `origin_url`, `meta`, `task_id`, `data_path`, `format_name`, `duration`, `codec_type`, `video_bit_rate`,`audio_bit_rate`, `probe_score`, `media_format_info`, `video_codec_info`, `audio_codec_info`, `tm`) values('%s', '%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s','%s', '%s', '%s', '%d', '%s', '%s', '%s', '%s');", 
			media_info_item.site,
			media_info_item.platform,
			media_info_item.program_type,
			media_info_item.resolution,
			media_info_item.origin_url,
			media_info_item.meta,
			media_info_item.task_id,
			media_info_item.data_path,
			media_info_item.format_name,
			media_info_item.duration,
			media_info_item.codec_type,
			media_info_item.video_bit_rate,
			media_info_item.audio_bit_rate,
			media_info_item.probe_score,
			media_info_item.media_format_info,
			media_info_item.video_codec_info,
			media_info_item.audio_codec_info,
		    time_format);
	int res = mysql_query(pconn, "SET NAMES utf8");
	res = mysql_query(pconn, sql);
	if (res) {
		printf("error occurd when inserting datas!\n");
		printf("The sql is : %s\n", sql);
	}else {
		printf("insert success!\n");
	}
}

int main() {
	//read the config
	config_t conf;
	config_init(&conf);
	if (!config_read_file(&conf, "basic.conf")) {
		fprintf(stderr, "%s:%d - %s\n", 
				config_error_file(&conf),
				config_error_line(&conf), 
				config_error_text(&conf));
		config_destroy(&conf);
		
		return -1;
	}

	const char* db_address;
	const char* db_user;
	const char* db_password;	
	const char* db_name;
	const char* data_path;
	if (!config_lookup_string(&conf, "db_address", &db_address)) {
		fprintf(stderr, "No 'db_address' setting in configuration file.\n");
	}
	if (!config_lookup_string(&conf, "db_user", &db_user)) {
		fprintf(stderr, "No 'db_address' setting in configuration file.\n");
	}
	if (!config_lookup_string(&conf, "db_password", &db_password)) {
		fprintf(stderr, "No 'db_password' setting in configuration file.\n");
	}
	if (!config_lookup_string(&conf, "db_name", &db_name)) {
		fprintf(stderr, "No 'db_name' setting in configuration file.\n");
	}
	if (!config_lookup_string(&conf, "data_path", &data_path)) {
		fprintf(stderr, "No 'data_path' setting in configuration file.\n");
	}
	
	//read the task
	MYSQL conn;
	mysql_init(&conn);
	//set reconnect true
	char value = 1;
	mysql_options(&conn, MYSQL_OPT_RECONNECT, &value);
	if (mysql_real_connect(&conn, db_address, db_user, db_password, db_name, 0, NULL, 0)) {
		int res = mysql_query(&conn, "select * from competitor_ondemand_analyse where be_picked=0 group by data_url order by id");
		if (!res) {
			printf("get data success!\n");
			MYSQL_RES *results = mysql_store_result(&conn);
			if (results) {
				MYSQL_ROW sql_row;
				string curr_task;
				MEDIA_INFO media_info_item;
				char file_path_split[128] = {'\0'};
				char file_path_split_prefix[128] = {'\0'};
				char file_list_line[128] = {'\0'};
				FILE *file = NULL;
				FILE *file_list = fopen("file_list.txt", "w");
				int split_num = 0;
				while(sql_row = mysql_fetch_row(results)) {
					// a new task
					if (curr_task.compare(sql_row[8]) != 0) {
						printf("got a new task to process!\n");
						//process the data
						int file_size = 0;
						if (0 < split_num) {
							task_processer(media_info_item, file_path_split_prefix, &conn);
						}
	
						//create a new task
						media_info_item.clear();
						strcpy(media_info_item.site, sql_row[1]);
						strcpy(media_info_item.platform, sql_row[2]);
						strcpy(media_info_item.resolution, sql_row[4]);
						strcpy(media_info_item.origin_url, sql_row[5]);
						strcpy(media_info_item.meta, sql_row[6]);
						strcpy(media_info_item.task_id, sql_row[8]);
						sprintf(media_info_item.data_path, "%stask_%s.bin", data_path, sql_row[8]);
						media_info_item.program_type = atoi(sql_row[3]);
						
						//reset task info
						split_num = 0;
						curr_task = sql_row[8];
						file_list = freopen("file_list.txt", "w", file_list);
					}
					split_num++;
					sprintf(file_path_split, "%stask_%s_%.4d.bin", data_path, sql_row[8], split_num);
					sprintf(file_path_split_prefix, "%stask_%s_*.bin", data_path, sql_row[8]);
					sprintf(file_list_line, "file '%s'\n", file_path_split);
					fwrite(file_list_line, sizeof(char), strlen(file_list_line), file_list);
					file = fopen(file_path_split, "w");
					//handle one sql line
					printf("%s:%s,%s,%s\n", sql_row[0], sql_row[1], sql_row[2],sql_row[8]);
					//retry 3 times
					for(int i = 0; i < 3; i++) {
						int res = request_get_withua(sql_row[7], file, sql_row[10]);
						if (CURLE_OK == res) {
							break;
						}
					}
					fclose(file);
					//首个分片的封装格式作为整体的格式，同时用于拼接文件
					if (1 == split_num) {
						char command[128] = {'\0'};
						sprintf(command, "ffprobe -show_format -print_format json %s", file_path_split);
						FILE *fd = popen(command, "r");
						if (fd) {
							char buffer[128] = {'\0'};
							char format_data[1024] = {'\0'};
							int pos = 0;
							while(!feof(fd)) {
								if (fgets(buffer, 128, fd) == NULL) {
									break;
								}
								strncpy(format_data + pos, buffer, strlen(buffer));
								pos += strlen(buffer);
							}
							char format[1024] = {'\0'};
							strtotoken(format_data, format, 1024, " \r\n");
							Json::Reader reader;
							Json::Value root;
							if (reader.parse(format, root)) {
								strcpy(media_info_item.media_format_info, format);
								strcpy(media_info_item.format_name, root["format"]["format_name"].asString().c_str());
								media_info_item.probe_score = root["format"]["probe_score"].asInt();
							}
							pclose(fd);
						}
					}
					//sleep in case of ...
					sleep(1);
				}
				if (NULL != file_list) {
					fclose(file_list);
				}
				if (0 < split_num) {
					task_processer(media_info_item, file_path_split_prefix, &conn);
				}
				if (mysql_errno(&conn)) {  
					fprintf(stderr, "Retrive error: %s\n", mysql_error(&conn)); 		
				} 
			}
			mysql_free_result(results);
		}else {
			fprintf(stderr, "SELECT error: %s\n", mysql_error(&conn));
		}
		//set the task status;
		mysql_query(&conn, "update competitor_ondemand_analyse set be_picked=1");
	}
	mysql_close(&conn);
	config_destroy(&conf);
	return 0;
}
