#ifndef __COMMON_H__
#define __COMMON_H__
#include <string.h>

typedef struct media_info {
	char site[16];
	char platform[16];
	char resolution[16];
	char origin_url[128];
	char meta[128];
	char task_id[64];
	char data_path[128];
	char format_name[64];
	char duration[128];
	char codec_type[16];
	char bit_rate[32];
	char video_bit_rate[32];
	char audio_bit_rate[32];
	char media_format_info[1024];
	char video_codec_info[4096];
	char audio_codec_info[4096];
	
	int program_type;
	int probe_score;

    void clear() {
		this->site[0] = '\0';
        this->platform[0] = '\0';
        this->resolution[0] = '\0';
        this->origin_url[0] = '\0';
        this->meta[0] = '\0';
        this->task_id[0] = '\0';
        this->data_path[0] = '\0';
        this->format_name[0] = '\0';
        this->duration[0] = '\0';
        this->codec_type[0] = '\0';
		this->bit_rate[0] = '\0';
        this->video_bit_rate[0] = '\0';
        this->audio_bit_rate[0] = '\0';
        this->media_format_info[0] = '\0';
        this->video_codec_info[0] = '\0';
        this->audio_codec_info[0] = '\0';
        
		this->program_type = 0;
        this->probe_score = 100;
    }
}MEDIA_INFO;

/**
 * @split and cat
 *
 */
char* strtotoken(char *src, char *dst, size_t length, const char *delimiters) {
    char *pch = strtok(src, delimiters);
    while(NULL != pch && strlen(dst) < length) {
		strcat(dst, pch);
        pch = strtok(NULL, delimiters);
    }
    return dst;
}


#endif

