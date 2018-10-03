#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include "bar_helpers.h"

#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) > (B) ? (B) : (A))
#define TIME_TO_WAIT 1
#define ICON_FA_VOLUME_UP u8"\uf028"
#define MAX_BLOCK_NUMBER 256
static char BAR_BG[] = "\"#1b1b1b\"";
static char BAR_FG[] = "\"#616161\"";
static char BAR_U[] = "\"#665c54\"";
static char BAR_FONT[] = "Fira Mono:pixelsize=10";
static char BAR_GEOM[] = "x30+0+0";
static int U_PIX = 5;
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
static block_param_t *blocks[MAX_BLOCK_NUMBER];
static block_container_t container;
static int number_of_desktops = 0;
static int number_of_blocks = 0;


static char *COMMANDS[] = {NULL, NULL, NULL, NULL, NULL};

static int run = 0;
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
				if (task_block != NULL) free_block_param_t(task_block);
				task_block = create_block_param_t(NULL, task_token, COLOR_FG_BG, TASK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, CENTER_ALIGN, TASK_PADDING, i);
				free(task_token);
	
			}
		}else{
			if (*token == 'F' || *token == 'O')
				desktop_tokens[tmp_number_of_desktops++] = strdup(token);
		}
	 	token = strtok(NULL, "\n");
	}
	
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
			desktop_blocks[i] = create_block_param_t(NULL, desktop_token, COLOR_FG_BG, DESKTOP_OCCUPIED_COLORS, NO_CLICK, COMMANDS, 0, NO_ATTR, LEFT_ALIGN, DESKTOP_PADDING, i+1);
		}
		free(desktop_tokens[i]);
	}
	number_of_desktops = tmp_number_of_desktops;
	
}
char *read_fd(int fd){
	size_t size = 0;
	size_t read_size = 0;
	char tmp_buff[1024] = {0};
	char *tmp_str = NULL;
	char *str = NULL;
	read_size = read(fd, tmp_buff, 1023);
	while (run){
		//printf("%d %d %d\n", read_size, errno, EAGAIN);
		if (read_size == -1 && errno == EAGAIN){
			return str;
		}
		else{
			//printf("%d %d %d\n", read_size, errno, EAGAIN);
			size += read_size;
			if (read_size == 1023){
				tmp_buff[read_size++] = '\0';
				size++;
				if (str == NULL){
					str = (char*)malloc((read_size)*sizeof(char));
					if (str == NULL){
						return NULL;
					}
					memcpy(str, tmp_buff, read_size);
				}
				else{//size-read_size -> 
					tmp_str = (char*)malloc((--size-read_size)*sizeof(char));
					memcpy(tmp_str, str, size-read_size);
					free(str);
					str = (char*)malloc(size*sizeof(char));
					if (str == NULL){
						free(tmp_str);
						return NULL;
					}
					memcpy(str, tmp_str, size - read_size);
					memcpy(str + size - read_size - 1, tmp_buff, read_size);
					free(tmp_str);
				}
			}else{
				if (tmp_buff[read_size-1] == '\n') tmp_buff[read_size-1] = '\0';
				else tmp_buff[read_size++] = '\0';
				if (str == NULL){
					str = (char*)malloc(read_size*sizeof(char));
					if (str == NULL){
						return NULL;
					}
					memcpy(str, tmp_buff, read_size);
				}
				else{
					tmp_str = (char*)malloc((--size-read_size)*sizeof(char));
					memcpy(tmp_str, str, size-read_size);
					free(str);
					str = (char*)malloc(size*sizeof(char));
					if (str == NULL){
						free(tmp_str);
						return NULL;
					}
					memcpy(str, tmp_str, size - read_size);
					memcpy(str + size - read_size - 1, tmp_buff, read_size);
					free(tmp_str);
				}
			}
			read_size = read(fd, tmp_buff, 1024);	
		}
		read_size = read(fd, tmp_buff, 1023);
	}
	return str;
}
void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        run = 0;
		
}
int main( int argc, char *argv[] )
{

	FILE *fp, *fstatus, *lemonbar_fd;
	char *buff = NULL;
	char *bar = NULL;
	/* Open the command for reading. */
	fp = popen("bumbar -c", "r");
	fstatus = popen("/home/branislav/bar_helpers/status.sh", "r");
	int d = fileno(fp);
	int s = fileno(fstatus);
	fcntl(d, F_SETFL, O_RDWR | O_NONBLOCK);
	fcntl(s, F_SETFL, O_RDWR | O_NONBLOCK);
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}
	if (fstatus == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}
	int sel_fd = MAX(d, s) + 1;
	fd_set descriptors;
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	run  = 1;
	char lemonbar_command[256] = {0};
	sprintf(lemonbar_command, "lemonbar -g %s -B %s -F %s -U %s -u %d -f %s", BAR_GEOM, BAR_BG, BAR_FG, BAR_U, U_PIX, BAR_FONT);
	lemonbar_fd = popen(lemonbar_command, "w");
	/*static char BAR_BG[] = "#1b1b1b";
static char BAR_FG[] = "#616161";
static char BAR_U[] = "#665c54";
static char BAR_FONT[] = "Fira Mono:pixelsize=12";
static char BAR_GEOM[] = "x30+0+0";
static int U_PIX = 5;*/
	while (run){

		FD_ZERO(&descriptors);
		FD_SET(d, &descriptors);
		FD_SET(s, &descriptors);
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(d, &descriptors)){
				/*memset(buff, 0, 1035);
				ssize_t r = read(d, buff, 1034);
				if (r == -1 && errno == EAGAIN)
				//printf("No data yet\n");
				continue;
				else if (r > 0){
				buff[r-1] = '\0';
				printf("Chunksize %d : %s", r,buff);}
				else
				printf("Pipe closed\n");*/
				if ((buff = read_fd(d)) != NULL){
					for (int i = 0; i < number_of_desktops; i++){
						free_block_param_t(desktop_blocks[i]);
						desktop_blocks[i] = NULL;
					}
					number_of_desktops = 0;
					get_desktop_and_task_blocks(buff, strlen(buff));
					get_clock_block();
					for (int i = 0; i < number_of_desktops; i++){
						blocks[i] = desktop_blocks[i];
					}
					number_of_blocks = number_of_desktops;
					if (task_block != NULL)
						blocks[number_of_blocks++] = task_block;
					if (clock_block != NULL)
						blocks[number_of_blocks++] = clock_block;
					if (volume_block != NULL)
						blocks[number_of_blocks++] = volume_block;
					if (battery_block != NULL)
						blocks[number_of_blocks++] = battery_block;
					if (brightness_block != NULL)
						blocks[number_of_blocks++] = brightness_block;
					if (wifi_block != NULL)
						blocks[number_of_blocks++] = wifi_block;
					create_block_container(&container, blocks, number_of_blocks, 0,0,0,0,1);
					bar = render_bar(NULL, &container);
					//printf("%s", bar);
					//fflush(stdout);
					fprintf(lemonbar_fd ,"%s", bar);
					fflush(lemonbar_fd);
					free(bar);
					free(buff);
				}
				
				
			}
			if (FD_ISSET(s, &descriptors)){
				/*memset(buff, 0, 1035);
				ssize_t r = read(s, buff, 1034);
				if (r == -1 && errno == EAGAIN)
				//printf("No data yet\n");
				continue;
				else if (r > 0){
				buff[r-1] = '\0';
				printf("Chunksize %d : %s\n", r,buff);}
				else
				printf("Pipe closed\n");*/
				if ((buff = read_fd(s)) != NULL){
					get_volume_block(buff, strlen(buff));
					for (int i = 0; i < number_of_desktops; i++){
						blocks[i] = desktop_blocks[i];
					}
					number_of_blocks = number_of_desktops;
					if (task_block != NULL)
						blocks[number_of_blocks++] = task_block;
					if (clock_block != NULL)
						blocks[number_of_blocks++] = clock_block;
					if (volume_block != NULL)
						blocks[number_of_blocks++] = volume_block;
					if (battery_block != NULL)
						blocks[number_of_blocks++] = battery_block;
					if (brightness_block != NULL)
						blocks[number_of_blocks++] = brightness_block;
					if (wifi_block != NULL)
						blocks[number_of_blocks++] = wifi_block;
					create_block_container(&container, blocks, number_of_blocks, 0,0,0,0,1);
					bar = render_bar(NULL, &container);
					//printf("%s", bar);
					//fflush(stdout);
					fprintf(lemonbar_fd ,"%s", bar);
					fflush(lemonbar_fd);
					free(bar);
					free(buff);
				}
			}
		}
	}
	/* close */
	printf("TERMINATING \n");
	for (int i=0; i<number_of_desktops;i++){
		free_block_param_t(desktop_blocks[i]);
	}
	free(desktop_blocks);
	if (task_block)
		free_block_param_t(task_block);
	if (clock_block)
		free_block_param_t(clock_block);
	if (volume_block)
		free_block_param_t(volume_block);
	if (battery_block)
		free_block_param_t(battery_block);
	if (brightness_block)
		free_block_param_t(brightness_block);
	pclose(fp); pclose(fstatus);pclose(lemonbar_fd);

	return 0;
}
