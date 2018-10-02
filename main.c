#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>  
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "bar_helpers.h"

#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) > (B) ? (B) : (A))
#define TIME_TO_WAIT 1
#define ICON_FA_VOLUME_UP u8"\uf028"
static  char FOCUSED_DESKTOP_FG[] = "#9e9e9e";
static  char FOCUSED_DESKTOP_BG[] = "#303030";
static  char FOCUSED_DESKTOP_UNDERLINE[] = "#665c54";
static  char OCCUPIED_DESKTOP_BG[] = "#1b1b1b";
static  char OCCUPIED_DESKTOP_FG[] = "#616161";
static  char ACTIVE_TASK_FG[] = "#9e9e9e";
static  char ACTIVE_TASK_BG[] = "#303030";
static  char *TASK_COLORS[3] = {ACTIVE_TASK_FG, ACTIVE_TASK_BG, NULL};
static  char *DESKTOP_FOCUSED_COLORS[3] = {FOCUSED_DESKTOP_FG, FOCUSED_DESKTOP_BG, FOCUSED_DESKTOP_UNDERLINE};
static  char *DESKTOP_OCCUPIED_COLORS[3] = {OCCUPIED_DESKTOP_FG, OCCUPIED_DESKTOP_BG, NULL};
static  char CLOCK_BG[] = "#303030";
static  char CLOCK_FG[] = "#9e9e9e";
static char *CLOCK_COLORS[3] = {CLOCK_FG, CLOCK_BG, NULL};
static const int TASK_TEXT_SIZE = 10;
static const int COLOR_FG_BG = BACKGROUND_C | FOREGROUND_C;
static const int COLOR_FG_BG_U = BACKGROUND_C | FOREGROUND_C | UNDERLINE_C;
static const int DESKTOP_PADDING = 2;
static const int TASK_PADDING = 2;
static const int CLOCK_PADDING = 2;
static block_param_t *clock_block = NULL;
static block_param_t **desktop_blocks = NULL;
static block_param_t *task_block = NULL;
static block_param_t *volume_block = NULL;
static block_param_t *battery_block = NULL;
static block_param_t *brightness_block = NULL;
static block_param_t *wifi_block = NULL;
static int number_of_desktops = 0;
static int number_of_blocks = 0;


static char *COMMANDS[] = {NULL, NULL, NULL, NULL, NULL};

static int run = 0;

/*block_param_t *create_block_param_t(block_param_t *dest, 
									char *text,//display text
									color_t colors,//which colors are in char array
									char **color_codes,//codes for used colors
									click_t clicks,//which clicks
									char **commands,//what commands are for needed clicks
									unsigned int font_index,//special font for block text
									attr_flag_t attr, //block flags
									attr_align_t aligment,//block aligment
									int padding,
									int position)//position of block in aligment)*/
int get_clock_block(void){

	time_t rawtime;
	struct tm *info;
	char buffer[1024];

	time( &rawtime );

	info = localtime( &rawtime );

	strftime(buffer,80,"%a, %b %d %H:%M", info);
	if (clock_block != NULL) free_block_param_t(clock_block);
	clock_block =  create_block_param_t(NULL, buffer, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, LEFT_ALIGN, CLOCK_PADDING, 0);
	
}

int get_battery_block(char *str, size_t size){
	char volume_str[1024] = {0};
	*volume_str = '\0';
	strcat(volume_str, "BAT: ");
	for (int i=0; i<size; i++){
		if (str[i]!='\n') volume_str[i+4] = str[i];
	}
	//sprintf(volume_str, "VOL: %s", str);
	if (battery_block != NULL) free_block_param_t(battery_block);
	battery_block =  create_block_param_t(NULL, volume_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 1);
}
int get_brightness_block(char *str, size_t size){
	char volume_str[1024] = {0};
	*volume_str = '\0';
	strcat(volume_str, "LUM: ");
	for (int i=0; i<size; i++){
		if (str[i]!='\n') volume_str[i+4] = str[i];
	}
	//sprintf(volume_str, "VOL: %s", str);
	if (brightness_block != NULL) free_block_param_t(brightness_block);
	brightness_block =  create_block_param_t(NULL, volume_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 2);
}
int get_wifi_block(char *str, size_t size){
	char volume_str[1024] = {0};
	*volume_str = '\0';
	for (int i=0; i<size; i++){
		if (str[i]!='\n') volume_str[i] = str[i];
	}
	if (wifi_block != NULL) free_block_param_t(wifi_block);
	wifi_block =  create_block_param_t(NULL, volume_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 0);
}
int get_volume_block(char *str, size_t size){

	char volume_str[1024] = {0};
	*volume_str = '\0';
	strcat(volume_str, "VOL: ");
	//strcat(volume_str, ICON_FA_VOLUME_UP);
	//strcat(volume_str, "%{-u} ");
	int len = strlen(volume_str);
	for (int i=0; i<size; i++){
		if (str[i]!='\n') volume_str[i+len] = str[i];
	}
	//sprintf(volume_str, "VOL: %s", str);
	if (volume_block != NULL) free_block_param_t(volume_block);
	volume_block =  create_block_param_t(NULL, volume_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 2);
}
int get_external_blocks(char*str, size_t size){
	char *token = strtok(str, ";");
	while(token != NULL){
		//printf("token : %s\n", token);
		if (*token == 'V')
			get_volume_block(token+1, strlen(token));
		if (*token == 'B')
			get_battery_block(token+1, strlen(token));
		if (*token == 'L')
			get_brightness_block(token+1, strlen(token));
		if (*token == 'W')
			get_wifi_block(token+1, strlen(token));
		token = strtok(NULL, ";");
	}
}									
int get_desktop_and_task_blocks(char str[], size_t str_len){
	
	char *token = strtok(str, "\n");
	char *desktop_tokens[20] = {NULL};
	char *task_token = NULL;
	char *desktop_token = NULL;
	size_t title_size = 0;
	int i = 0;
	int tmp_number_of_desktops = 0;
	while (token != NULL){
		if (*token == '\t'){
			token++;
			if (*token == 'F'){
				token = token + 2;
				token = strchr(token, ':');
				token++;
				title_size = strlen(token);
				task_token = (char*)malloc(title_size+1);
				memcpy(task_token, token, title_size + 1);
			}
		}else{
			if (*token == 'F' || *token == 'O')
				desktop_tokens[tmp_number_of_desktops++] = strdup(token);
		}
	 	token = strtok(NULL, "\n");
	}
	if (task_block != NULL) free_block_param_t(task_block);
	if (task_token != NULL)
		task_block = create_block_param_t(NULL, task_token, COLOR_FG_BG, TASK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, CENTER_ALIGN, TASK_PADDING, i);
	else
		task_block = create_block_param_t(NULL, "EMPTY", COLOR_FG_BG, TASK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, CENTER_ALIGN, TASK_PADDING, i);
	free(task_token);
	for (i = 0; i < number_of_desktops; i++){
		if (desktop_blocks[i] != NULL) free_block_param_t(desktop_blocks[i]);
	}
	if (desktop_blocks != NULL){
		free(desktop_blocks);
	}
	desktop_blocks = (block_param_t**)malloc(tmp_number_of_desktops*sizeof(block_param_t*));
	for (i = 0; i < tmp_number_of_desktops; i++){
		desktop_token = desktop_tokens[i];
		if (desktop_token[0] == 'F'){
			desktop_token = strchr(desktop_token + 2, ':');
			desktop_token++;
			desktop_blocks[i] = create_block_param_t(NULL, desktop_token, COLOR_FG_BG, DESKTOP_FOCUSED_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, LEFT_ALIGN, DESKTOP_PADDING, i+1);
		}else 
		if(desktop_token[0] == 'O'){
		//occupied
			desktop_token = strchr(desktop_token + 2, ':');
			desktop_token++;
			desktop_blocks[i] = create_block_param_t(NULL, desktop_token, COLOR_FG_BG, DESKTOP_OCCUPIED_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, LEFT_ALIGN, DESKTOP_PADDING, i+1);
		}
		free(desktop_tokens[i]);
	}
	number_of_desktops = tmp_number_of_desktops;
	
}
void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        run = 0;
		
}

int main(int argc, char *argv[]){
	char tmp[1024];
	char **tmp_arr =  (char**)malloc(256*sizeof(char*));
	char *complete = NULL;
	char fifo_name1[] = "/tmp/bumbar_fifo";
	char fifo_name2[] = "/tmp/bar_internal_fifo";
	int s2c= open(fifo_name1, O_RDWR | O_NONBLOCK);
	int s3c= open(fifo_name2, O_RDWR | O_NONBLOCK);
	int i, j;
	size_t size = 0;
	char *token_start;
	char *token_end;
	char *token;
	int over = 0;
	char **block_arr = NULL;
	int read_size = 0;
	char *desktop[4];
	char **clients = NULL;
	block_param_t **blocks;
	block_container_t *container = NULL;
	char *bar = NULL;
	int sel_fd = MAX(s2c, s3c) + 1;
	fd_set descriptors;
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	run = 1;
	while (run)
	{
		FD_ZERO(&descriptors);
		FD_SET(s2c, &descriptors);
		FD_SET(s3c, &descriptors);
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(s2c, &descriptors)){
				i=0;
				memset(tmp,0, 1024);
				*tmp = '\0';
				while (read(s2c, &tmp, 1024*sizeof(char)) > 0)
				{	
			
					
					read_size = strlen(tmp) + 1;
					tmp_arr[i] = strdup(tmp);
					if (i==0) size = 0;
					size += read_size;
					i++;
					memset(tmp,0, 1024);
					*tmp = '\0';
				}
				complete = (char*)malloc(size*sizeof(char));
				memset(complete, 0, size*sizeof(char));
				*complete = '\0'; 
				for (int j=0; j < i; j++){
					//size += strlen(tmp_arr[j])
					//printf("%s", tmp_arr[j]);
					strcat(complete, tmp_arr[j]);
					free(tmp_arr[j]);
				}
				//printf("%s\n\n", complete);
				get_desktop_and_task_blocks(complete, size);
				get_clock_block();
				blocks = (block_param_t **)malloc((number_of_desktops+6)*sizeof(block_param_t *));
				for (i = 0; i < number_of_desktops; i++){
					blocks[i] = desktop_blocks[i];
				}
				number_of_blocks = number_of_desktops;
				blocks[number_of_blocks++] = task_block;
				blocks[number_of_blocks++] = clock_block;
				if (volume_block != NULL)
					blocks[number_of_blocks++] = volume_block;
				if (battery_block != NULL)
					blocks[number_of_blocks++] = battery_block;
				if (brightness_block != NULL)
					blocks[number_of_blocks++] = brightness_block;
				if (wifi_block != NULL)
					blocks[number_of_blocks++] = wifi_block;
				container = create_block_container(NULL, blocks, number_of_blocks, 0,0,0,0,1);
				bar = render_bar(NULL, container);
				printf("%{+u}%s", bar);
				fflush(stdout);
				free(bar);
				free(container);
				free(complete);
				free(blocks);
			}
			if (FD_ISSET(s3c, &descriptors)){
				i=0;
				memset(tmp,0, 1024);
				*tmp = '\0';
				while (read(s3c, &tmp, 1024*sizeof(char)) > 0)
				{	
			
					//printf("%c", buf);
					printf("%s", tmp);
					read_size = strlen(tmp) + 1;
					tmp_arr[i] = strdup(tmp);
					if (i==0) size = 0;
					size += read_size;
					i++;
					memset(tmp,0, 1024);
					*tmp = '\0';
				}
				complete = (char*)malloc(size*sizeof(char));
				memset(complete, 0, size*sizeof(char));
				*complete = '\0'; 
				for (int j=0; j < i; j++){
					//size += strlen(tmp_arr[j])
					//printf("%s", tmp_arr[j]);
					strcat(complete, tmp_arr[j]);
					free(tmp_arr[j]);
				}
				get_clock_block();
				//get_volume_block(complete, size);
				//get_battery_block(complete, size);
				//get_brightness_block(complete, size);
				get_external_blocks(complete, size);
				blocks = (block_param_t **)malloc((number_of_desktops+6)*sizeof(block_param_t *));
				for (i = 0; i < number_of_desktops; i++){
					blocks[i] = desktop_blocks[i];
				}
				number_of_blocks = number_of_desktops;
				blocks[number_of_blocks++] = clock_block;
				if (task_block != NULL)
					blocks[number_of_blocks++] = task_block;
				if (volume_block != NULL)
					blocks[number_of_blocks++] = volume_block;
				if (battery_block != NULL)
					blocks[number_of_blocks++] = battery_block;
				if (brightness_block != NULL)
					blocks[number_of_blocks++] = brightness_block;
				if (wifi_block != NULL)
					blocks[number_of_blocks++] = wifi_block;
				container = create_block_container(NULL, blocks, number_of_blocks, 0,0,0,0,1);
				bar = render_bar(NULL, container);
				printf("%{+u}%s", bar);
				fflush(stdout);
				free(bar);
				free(container);
				free(complete);
				free(blocks);
				
			}
		}
		
	}
	for (i=0; i<number_of_desktops;i++){
		free_block_param_t(desktop_blocks[i]);
	}
	free(desktop_blocks);
	free_block_param_t(task_block);
	free_block_param_t(clock_block);
	free_block_param_t(volume_block);
	free_block_param_t(battery_block);
	free_block_param_t(brightness_block);
	close(s2c);
	free(tmp_arr);
	printf("client exit successfully");
    return EXIT_SUCCESS;
}
