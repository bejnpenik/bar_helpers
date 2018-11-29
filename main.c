#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include "ICON_CODES.h"
#include "ICON_FA_CODES.h"
#include "bar_helpers.h"
#define READ 0
#define WRITE 1
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) > (B) ? (B) : (A))
#define MAX_BLOCK_NUMBER 256
static char UNLOCK_ROT_CMD[] = "unlock_rotation";
static char LOCK_ROT_CMD[] = "lock_rotation";
static char TOGGLE_ROT_CMD[] = "unlock_rotation";
static char BAR_BG[] = "#1b1b1b";
static char BAR_FG[] = "#616161";
static char BAR_U[] = "#665c54";
static char BAR_FONT[] = "Fira Mono:style=Regular:pixelsize=10";
static char ICON_FONT[] = "Material Icons:style=Regular:pixelsize=14";
static char FA_FONT_REG[] = "Font Awesome 5 Free:style=Regular:pixelsize=14";
static char FA_FONT_SOL[] = "Font Awesome 5 Free:style=Solid:pixelsize=14";
static char FA_FONT_BRANDS[] = "Font Awesome 5 Brands:pixelsize=14";
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
static const int APP_PADDING = 3;
static block_param_t *clock_block = NULL;
static block_param_t **desktop_blocks = NULL;
static block_param_t *task_block = NULL;
static block_param_t *volume_block = NULL;
static block_param_t *battery_block = NULL;
static block_param_t *brightness_block = NULL;
static block_param_t *wifi_block = NULL;
static block_param_t *app_launcher_block = NULL;
static block_param_t *app_switcher_block = NULL;
static block_param_t *rotation_block = NULL;
static block_param_t *keyboard_block = NULL;
static block_param_t *updates_block = NULL;
static block_param_t *blocks[MAX_BLOCK_NUMBER];
static block_container_t container;
static int number_of_desktops = 0;
static int number_of_blocks = 0;
static int number_of_external_blocks = 11;

static char *socket_path = NULL;
static size_t socket_path_len = 0;

typedef enum {
	HORIZONTAL,
	VERTICAL
} display_orientation_t;
typedef enum {
	CLOCK,
	VOLUME,
	BATTERY,
	UPDATES,
	LUMINOSITY,
	WIFI,
	DESKTOP
} block_query_t;

static char *COMMANDS[] = {NULL, NULL, NULL, NULL, NULL};
static char *APP_LAUNCHER_COMMANDS[] = {"rofi -modi drun -show drun -location 1 -width 30 -theme material -show-icons -xoffset 10 -yoffset 30 -no-click-to-exit", NULL, NULL, NULL, NULL};
static char *APP_SWITCHER_COMMANDS[] = {"rofi -show window -theme material -show-icons -no-click-to-exit", NULL, NULL, NULL, NULL};
static char *ROTATION_COMMANDS[] = {"ROTATION", NULL, NULL, NULL, NULL};
static char *KEYBOARD_COMMANDS[] = {"KEYBOARD", NULL, NULL, NULL, NULL};
static char keyboard_show[] = "dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Show";
static char keyboard_hide[] = "dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Hide";
static char bspc_padding_reclaim[] = "bspc config bottom_padding 0";
static char bspc_padding_add[] = "bspc config bottom_padding 279";
static char bspc_padding_add_vertical[] = "bspc config bottom_padding 200";
static int rotation_state[2] = {0};
static int keyboard_state[2] = {0};
static int orientation = -1;
static int text_size = 0;
static int pixel_size = 12;
static int run = 0;
pid_t popen2(char * const command[], int *infp, int *outfp)
{
    int p_stdin[2], p_stdout[2];
    pid_t pid;
    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], READ);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], WRITE);

        execvp(*command, command);
        perror("execvp");
        exit(1);
    }

    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];

    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];

    return pid;
}
int send_cmd_to_server(char *cmd, size_t cmd_len, char *socket_path, size_t path_len){
	int sockfd, servlen,n;
	struct sockaddr_un  serv_addr;
	serv_addr.sun_family = AF_UNIX;
	if (socket_path == NULL){
		strcpy(serv_addr.sun_path,"/tmp/2in1screen.socket");
	}else{
		memcpy(serv_addr.sun_path,socket_path, path_len);
	}
	servlen = strlen(serv_addr.sun_path) + 
		         sizeof(serv_addr.sun_family);
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
		fprintf(stderr,"Creating socket");
	if (connect(sockfd, (struct sockaddr *) 
		             &serv_addr, servlen) < 0)
		fprintf(stderr, "Connecting");
	if (send(sockfd,cmd, cmd_len, 0) == -1){
		fprintf(stderr, "Failed to send the data.\n"); 
	}
	char buffer[256] = {0};
	n=read(sockfd,buffer,255);
	buffer[n] = '\0';
	if (strncmp(buffer, "STATUS OK", 9) != 0){
		fprintf(stderr, "BAD DATA.\n %s\n", buffer);
	}
	close(sockfd);	
}

int get_app_launcher(void){
	char app_str[24] = {0};
	app_str[0] = '\0';
	strcat(app_str, ICON_MD_APPS);
	if (app_launcher_block != NULL) free_block_param_t(app_launcher_block);
	app_launcher_block = create_block_param_t(NULL, app_str, COLOR_FG_BG, CLOCK_COLORS, LEFT_CLICK, APP_LAUNCHER_COMMANDS, 0, UNDERLINE, LEFT_ALIGN, APP_PADDING, 0);
	return strlen(app_str)+2*APP_PADDING;
}
int get_app_switcher(void){
	char app_str[24] = {0};
	app_str[0] = '\0';
	strcat(app_str, ICON_MD_MENU);
	if (app_switcher_block != NULL) free_block_param_t(app_switcher_block);
	app_switcher_block = create_block_param_t(NULL, app_str, COLOR_FG_BG, CLOCK_COLORS, LEFT_CLICK, APP_SWITCHER_COMMANDS, 0, UNDERLINE, LEFT_ALIGN, APP_PADDING, 1);
	return strlen(app_str)+2*APP_PADDING;
}
int get_screen_rotation(void){
	char app_str[24] = {0};
	app_str[0] = '\0';
	if (rotation_state[1] == 1)
		strcat(app_str, ICON_MD_SCREEN_LOCK_ROTATION);
	else if (rotation_state[1]==0)
		strcat(app_str, ICON_MD_SCREEN_ROTATION);
	if (rotation_block != NULL) free_block_param_t(rotation_block);
	rotation_block = create_block_param_t(NULL, app_str, COLOR_FG_BG, CLOCK_COLORS, LEFT_CLICK, ROTATION_COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, APP_PADDING, 4);
	return strlen(app_str)+2*APP_PADDING;
}
int set_rotation_state(void){
	if (rotation_state[0] == 0){
		rotation_state[1] = 1;
		//printf("LOCKED");
		//fflush(stdout);
		send_cmd_to_server(LOCK_ROT_CMD, strlen(LOCK_ROT_CMD), socket_path, socket_path_len);
	}
	else if (rotation_state[0] == 1){
		rotation_state[1] = 0;
		//printf("UNLOCKED");
		//fflush(stdout);
		send_cmd_to_server(UNLOCK_ROT_CMD, strlen(UNLOCK_ROT_CMD), socket_path, socket_path_len);
	}
	rotation_state[0] = rotation_state[1];
}
int get_keyboard_block(void){
	char app_str[24] = {0};
	app_str[0] = '\0';
	if (keyboard_state[1] == 1)
		strcat(app_str, ICON_MD_KEYBOARD);
	else if (keyboard_state[1]==0)
		strcat(app_str, ICON_MD_KEYBOARD_HIDE);
	if (keyboard_block != NULL) free_block_param_t(keyboard_block);
	keyboard_block = create_block_param_t(NULL, app_str, COLOR_FG_BG, CLOCK_COLORS, LEFT_CLICK, KEYBOARD_COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, APP_PADDING, 5);
	return strlen(app_str)+2*APP_PADDING;
}
int set_keyboard_state(void){
	if (keyboard_state[0] == 0){
		keyboard_state[1] = 1;
		system(keyboard_hide);
		system(bspc_padding_reclaim);
	}
	else if (keyboard_state[0] == 1){
		keyboard_state[1] = 0;
		system(keyboard_show);
		if (orientation == VERTICAL) system(bspc_padding_add_vertical);
		else
			system(bspc_padding_add);
	}
	keyboard_state[0] = keyboard_state[1];
}
int get_clock_block(char *str, size_t size){

	char clock_str[100] = {0};
	*clock_str = '\0';
	int len = strlen(clock_str);
	for (int i=0; i<size; i++){
		if (str[i]!='\n') clock_str[len++] = str[i];
	}
	clock_str[len] = '\0';
	if (clock_block != NULL) free_block_param_t(clock_block);
	clock_block =  create_block_param_t(NULL, clock_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 6);
	return strlen(clock_str)+2*CLOCK_PADDING;
	
}
int get_updates_block(char *str, size_t size){
	
	char update_str[100] = {0};
	*update_str = '\0';
	strcat(update_str, ICON_FA_SYNC_ALT);
	strcat(update_str, " ");
	int len = strlen(update_str);
	for (int i=0; i<size; i++){
		if (str[i]!='\n') update_str[len++] = str[i];
	}
	update_str[len] = '\0';
	if (updates_block != NULL) free_block_param_t(updates_block);
	updates_block =  create_block_param_t(NULL, update_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 1);

}
int get_volume_block(char *str, size_t size){

	char volume_str[1024] = {0};
	*volume_str = '\0';
	int level;
	if (strncmp(str, "Muted", 5)==0){
		strcat(volume_str, ICON_FA_VOLUME_OFF);
		strcat(volume_str, " ");
		//strcat(volume_str, ICON_FA_VOLUME_UP);
		//strcat(volume_str, "%{-u} ");
		
	}
	else{
		level = atoi(str);
		if (level > 50){
			strcat(volume_str, ICON_FA_VOLUME_UP);
			strcat(volume_str, " ");
		}else if (level == 0){
			strcat(volume_str, ICON_FA_VOLUME_OFF);
			strcat(volume_str, " ");
		}
		else{
			strcat(volume_str, ICON_FA_VOLUME_DOWN);
			strcat(volume_str, " ");
		}
	}
	int len = strlen(volume_str);
	for (int i=0; i<size; i++){
		if (str[i]!='\n') volume_str[len++] = str[i];
	}
	volume_str[len] = '\0';
	if (volume_block != NULL) free_block_param_t(volume_block);
	volume_block =  create_block_param_t(NULL, volume_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 4);
}

int get_battery_block(char *str, size_t size){

	char battery_str[1024] = {0};
	char *status = NULL;
	char *level_str = NULL;
	status = strtok(str, ":");
	level_str = strtok(NULL, ":");
	size = strlen(level_str);
	int level = atoi(level_str);
	*battery_str = '\0';
	if (strcmp(status, "Charging") == 0){
		strcat(battery_str, ICON_FA_PLUG);
		strcat(battery_str, " ");	
	}
	else{
		if (level >= 85){
			strcat(battery_str, ICON_FA_BATTERY_FULL);
			strcat(battery_str, " ");
		}else
		if (level<85 && level >= 65){
			strcat(battery_str, ICON_FA_BATTERY_THREE_QUARTERS);
			strcat(battery_str, " ");
		}else 
		if (level<65 && level >= 45){
			strcat(battery_str, ICON_FA_BATTERY_HALF);
			strcat(battery_str, " ");
		}else
		if (level >= 15 && level < 45){
			strcat(battery_str, ICON_FA_BATTERY_QUARTER);
			strcat(battery_str, " ");
		}else
		if (level < 15){
			strcat(battery_str, ICON_FA_BATTERY_EMPTY);
			strcat(battery_str, " ");
		}
	}
	int len = strlen(battery_str);
	for (int i=0; i<size; i++){
		if (level_str[i]!='\n' || level_str[i] != '\0') battery_str[len++] = level_str[i];
	}
	battery_str[len]='%';
	battery_str[len+1]='\0';
	//sprintf(volume_str, "VOL: %s", str);
	if (battery_block != NULL) free_block_param_t(battery_block);
	battery_block =  create_block_param_t(NULL, battery_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 3);
}
int get_luminosity_block(char *str, size_t size){

	char luminosity_str[1024] = {0};
	*luminosity_str = '\0';
	int level = atoi(str);
	if (level <= 10){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_1);
		strcat(luminosity_str, " ");
	}else
	if (level>10 && level<=20){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_2);
		strcat(luminosity_str, " ");
	}else
	if (level>20 && level<=30){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_3);
		strcat(luminosity_str, " ");
	}else
	if (level>30 && level<=40){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_4);
		strcat(luminosity_str, " ");
	}else
	if (level>40 && level<=50){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_4);
		strcat(luminosity_str, " ");
	}else
	if (level>50 && level<=60){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_5);
		strcat(luminosity_str, " ");
	}else
	if (level>60 && level<=70){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_6);
		strcat(luminosity_str, " ");
	}else
	if (level>70 && level<=80){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_6);
		strcat(luminosity_str, " ");
	}else
	if (level>80 && level<=90){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_7);
		strcat(luminosity_str, " ");
	}else
	if (level>90 && level<=100){
		strcat(luminosity_str, ICON_MD_BRIGHTNESS_7);
		strcat(luminosity_str, " ");
	}
	//strcat(volume_str, ICON_FA_VOLUME_UP);
	//strcat(volume_str, "%{-u} ");
	int len = strlen(luminosity_str);
	for (int i=0; i<size; i++){
		if (str[i]!='\n' || str[i] != '\0') luminosity_str[len++] = str[i];
	}
	luminosity_str[len]='%';
	luminosity_str[len+1]='\0';
	//sprintf(volume_str, "VOL: %s", str);
	if (brightness_block != NULL) free_block_param_t(brightness_block);
	brightness_block =  create_block_param_t(NULL, luminosity_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 2);
}

int get_wifi_block(char *str, size_t size){

	char wifi_str[1024] = {0};
	*wifi_str = '\0';
	strcat(wifi_str, ICON_FA_WIFI);
	strcat(wifi_str, " ");
	int len = strlen(wifi_str);
	for (int i=0; i<size; i++){
		if (str[i]!='\n' || str[i] != '\0') wifi_str[len++] = str[i];
	}
	wifi_str[len]='\0';
	//sprintf(volume_str, "VOL: %s", str);
	if (wifi_block != NULL) free_block_param_t(wifi_block);
	wifi_block =  create_block_param_t(NULL, wifi_str, COLOR_FG_BG, CLOCK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, RIGHT_ALIGN, CLOCK_PADDING, 0);
}
int get_desktop_and_task_blocks(char str[], size_t str_len){
	//SOMETIMES THERE IS DOUBLE OUTPUT DUE TO EVENT BEING PROCESSED
	char *token = strtok(str, "\n");
	char *desktop_tokens[20] = {NULL};
	char *task_token = NULL;
	char *desktop_token = NULL;
	size_t title_size = 0;
	int i = 0;
	int tmp_number_of_desktops = 0;
	int task_available = 0;
	int truncated = 0;
	int text_count = 0;
	while (token != NULL){
		if (*token == '\t'){
			token++;
			if (*token == 'F'){
				truncated = 0;
				token = token + 2;
				token = strchr(token, ':');
				token++;
				title_size = strlen(token);
				if (title_size  > 30) {
					title_size = 30;
					truncated = 1;
				}
				
				task_token = (char*)malloc(title_size+4);
				memcpy(task_token, token, title_size);
				task_token[title_size] = '\0';
				if (truncated){
					strcat(task_token, "...");
				}
				task_available = 1;
				if (task_block != NULL) free_block_param_t(task_block);
				task_block = create_block_param_t(NULL, task_token, COLOR_FG_BG, TASK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, CENTER_ALIGN, TASK_PADDING, 0);
				free(task_token);
			}
		}else{
			if (*token == 'F' || *token == 'O')
				desktop_tokens[tmp_number_of_desktops++] = strdup(token);
		}
	 	token = strtok(NULL, "\n");
	}
	if (task_available == 0) {
		if (task_block != NULL) free_block_param_t(task_block);
		task_block = NULL;
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
			desktop_blocks[i] = create_block_param_t(NULL, desktop_token, COLOR_FG_BG, DESKTOP_FOCUSED_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, LEFT_ALIGN, DESKTOP_PADDING, i+3);
			text_count += strlen(desktop_token) + 2*DESKTOP_PADDING;
		}else 
		if(desktop_token[0] == 'O'){
		//occupied
			
			desktop_token = strchr(desktop_token + 2, ':');
			desktop_token++;
			desktop_blocks[i] = create_block_param_t(NULL, desktop_token, COLOR_FG_BG, DESKTOP_OCCUPIED_COLORS, NO_CLICK, COMMANDS, 0, NO_ATTR, LEFT_ALIGN, DESKTOP_PADDING, i+3);
			text_count += strlen(desktop_token) + 2*DESKTOP_PADDING;
		}
		free(desktop_tokens[i]);
	}
	number_of_desktops = tmp_number_of_desktops;
	return text_count;
}
int get_desktop_and_task_blocks_vertical(char str[], size_t str_len){
	//SOMETIMES THERE IS DOUBLE OUTPUT DUE TO EVENT BEING PROCESSED
	char *token = strtok(str, "\n");
	char *desktop_tokens[20] = {NULL};
	char *task_token = NULL;
	char *desktop_token = NULL;
	size_t title_size = 0;
	int i = 0;
	int tmp_number_of_desktops = 0;
	int task_available = 0;
	int truncated = 0;
	while (token != NULL){
		if (*token == '\t'){
			token++;
			if (*token == 'F'){
				truncated = 0;
				token = token + 2;
				token = strchr(token, ':');
				token++;
				title_size = strlen(token);
				if (title_size  > 30) {
					title_size = 30;
					truncated = 1;
				}
				
				task_token = (char*)malloc(title_size+4);
				memcpy(task_token, token, title_size);
				task_token[title_size] = '\0';
				if (truncated){
					strcat(task_token, "...");
				}
				if (task_block != NULL) free_block_param_t(task_block);
				task_block = create_block_param_t(NULL, task_token, COLOR_FG_BG, TASK_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, LEFT_ALIGN, TASK_PADDING, 4);
				free(task_token);
				task_available = 0;
	
			}
		}else{
			if (*token == 'F' || *token == 'O')
				desktop_tokens[tmp_number_of_desktops++] = strdup(token);
		}
	 	token = strtok(NULL, "\n");
	}
	if (task_available == 0) {
		if (task_block != NULL) free_block_param_t(task_block);
		task_block = NULL;
	}
	
	for (i = 0; i < number_of_desktops; i++){
		if (desktop_blocks[i] != NULL) free_block_param_t(desktop_blocks[i]);
	}
	if (desktop_blocks != NULL){
		free(desktop_blocks);
	}
	desktop_blocks = (block_param_t**)malloc(sizeof(block_param_t*));
	for (i = 0; i < tmp_number_of_desktops; i++){
		desktop_token = desktop_tokens[i];
		if (desktop_token[0] == 'F'){
			desktop_token = strchr(desktop_token + 2, ':');
			desktop_token++;
			desktop_blocks[0] = create_block_param_t(NULL, desktop_token, COLOR_FG_BG, DESKTOP_FOCUSED_COLORS, NO_CLICK, COMMANDS, 0, UNDERLINE, LEFT_ALIGN, DESKTOP_PADDING, 3);
		}
		free(desktop_tokens[i]);
	}
	number_of_desktops = 1;
	
}
char *read_fd(int fd){
	size_t size = 0;
	size_t read_size = 0;
	char tmp_buff[1024] = {0};
	char *tmp_str = NULL;
	char *str = NULL;
	read_size = read(fd, tmp_buff, 1023);
	while (run){
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

int get_block(char *block_query){
	if (strncmp(block_query, "DES", 3) == 0) return DESKTOP;
	if (strncmp(block_query, "CLK", 3) == 0) return CLOCK;
	if (strncmp(block_query, "VOL", 3) == 0) return VOLUME;
	if (strncmp(block_query, "BAT", 3) == 0) return BATTERY;
	if (strncmp(block_query, "UPD", 3) == 0) return UPDATES;
	if (strncmp(block_query, "WII", 3) == 0) return WIFI;
	if (strncmp(block_query, "LUM", 3) == 0) return LUMINOSITY;
}
void parse_query(char *buff){
	char *query = NULL;
	char *output = NULL;
	query = strtok(buff, ";");
	output = strtok(NULL, ";");
	int response = get_block(query);
	switch (response){
		case DESKTOP:
			for (int i = 0; i < number_of_desktops; i++){
				free_block_param_t(desktop_blocks[i]);
				desktop_blocks[i] = NULL;
			}
			if (orientation == VERTICAL){
				get_desktop_and_task_blocks_vertical(output, strlen(output));
			}else
				get_desktop_and_task_blocks(output, strlen(output));
			break;
		case CLOCK:
			get_clock_block(output, strlen(output));
			break;
		case VOLUME:
			get_volume_block(output, strlen(output));
			break;
		case BATTERY:
			get_battery_block(output, strlen(output));
			break;
		case UPDATES:
			get_updates_block(output, strlen(output));
			break;
		case LUMINOSITY:
			get_luminosity_block(output, strlen(output));
			break;
		case WIFI:
			get_wifi_block(output, strlen(output));
			break;
		default:
			break;
			
			
			
	}
		
}
void set_blocks(void){
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
	if (app_launcher_block != NULL)
		blocks[number_of_blocks++] = app_launcher_block;
	if (app_switcher_block != NULL)
		blocks[number_of_blocks++] = app_switcher_block;
	if (rotation_block != NULL)
		blocks[number_of_blocks++] = rotation_block;
	if (keyboard_block != NULL)
		blocks[number_of_blocks++] = keyboard_block;
	if (updates_block != NULL)
		blocks[number_of_blocks++] = updates_block;
}
void cleanup(void){
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
	if (brightness_block)
		free_block_param_t(wifi_block);	
	if (app_launcher_block != NULL)
		free_block_param_t(app_launcher_block);
	if (app_switcher_block != NULL)
		free_block_param_t(app_switcher_block);
	if (rotation_block != NULL)
		free_block_param_t(rotation_block);
	if (keyboard_block != NULL)
		free_block_param_t(keyboard_block);
	if (updates_block != NULL)
		free_block_param_t(updates_block);
	
}
int main(int argc, char *argv[] ){
	if (argc > 1){
		for (int  i = 1; i < argc; i++){
			if (strncmp(argv[i], "--orientation", 13) == 0){
				if (strncmp(argv[i+1],"horizontal", 10)==0) orientation = HORIZONTAL;
				if (strncmp(argv[i+1], "vertical", 8)==0) orientation = VERTICAL;
			}else
			if (strncmp(argv[i], "--socket-path", 13) == 0){
				socket_path_len = strlen(argv[i+1]);
				socket_path = (char *)malloc((socket_path_len+ 1)*sizeof(char));
				memcpy(socket_path, argv[i+1], socket_path_len);
				socket_path[socket_path_len] = '\0';
			}
		}
		
	}
	char *buff = NULL;
	char *bar = NULL;
	int i;
	char bumbar_fifo[] = "/tmp/bumbar_fifo";
	int d = open(bumbar_fifo, O_RDWR | O_NONBLOCK);
	int lemon_write_fd, lemon_read_fd;
	char *lemon_comm[] = {"lemonbar", "-g", BAR_GEOM, "-B", BAR_BG, "-F", BAR_FG, "-U", BAR_U, "-u", "5", "-f",  BAR_FONT, "-f", ICON_FONT, "-f", FA_FONT_REG, "-f", FA_FONT_SOL, "-f", FA_FONT_BRANDS, "-a", "100", NULL};
	pid_t lemons = popen2(lemon_comm, &lemon_write_fd, &lemon_read_fd);
	fcntl(lemon_read_fd, F_SETFL, O_RDWR | O_NONBLOCK);
	int sel_fd = MAX(d, lemon_read_fd);
	char *clock_comm[] = {"/home/branko/.bin/bar_utils/clock", "-s", NULL};
	char *vol_comm[] = {"/home/branko/.bin/bar_utils/pulseaudio.sh", "--output_volume_listener", NULL};
	char *bat_comm[] = {"/home/branko/.bin/bar_utils/bat", NULL};
	char *lum_comm[] = {"/home/branko/.bin/bar_utils/lum", NULL};
	char *wifi_comm[] = {"/home/branko/.bin/bar_utils/essid", "-s", "-w", "wlo1", NULL};
	char *updates_comm[] = {"/home/branko/.bin/bar_utils/updates.sh", NULL};
	char **external_comm[] = {clock_comm, vol_comm, bat_comm, lum_comm, wifi_comm, updates_comm};
	int external_comm_len = 6;
	int *external_comm_fd = (int*)malloc(external_comm_len*sizeof(int));
	pid_t *external_comm_pid = (pid_t *)malloc(external_comm_len*sizeof(pid_t));
	for (i=0;i<external_comm_len;i++){
		external_comm_pid[i] = popen2(external_comm[i], NULL, external_comm_fd+i);
		fcntl(external_comm_fd[i], F_SETFL, O_RDWR | O_NONBLOCK);
		sel_fd = MAX(sel_fd, external_comm_fd[i]);
	}	
	sel_fd++;
	fd_set descriptors;
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	run  = 1;
	get_app_launcher();
	get_app_switcher();
	get_screen_rotation();
	get_keyboard_block();
	while (run){
		FD_ZERO(&descriptors);
		FD_SET(d, &descriptors);
		FD_SET(lemon_read_fd, &descriptors);
		for (i = 0; i < external_comm_len; i++){
			FD_SET(external_comm_fd[i], &descriptors);
		}
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(d, &descriptors)){
				if ((buff = read_fd(d)) != NULL){
					for (int i = 0; i < number_of_desktops; i++){
						free_block_param_t(desktop_blocks[i]);
						desktop_blocks[i] = NULL;
					}
					number_of_desktops = 0;
					if (orientation == VERTICAL){
						get_desktop_and_task_blocks_vertical(buff, strlen(buff));
					}else
						get_desktop_and_task_blocks(buff, strlen(buff));
					set_blocks();
					create_block_container(&container, blocks, number_of_blocks, 0, 0,0,0,1);
					bar = render_bar(NULL, &container);
					write(lemon_write_fd, bar, strlen(bar));
					free(bar);
					free(buff);
				}
			}
			if (FD_ISSET(lemon_read_fd, &descriptors)){
					if ((buff = read_fd(lemon_read_fd)) != NULL){
						if (strcmp(buff, "ROTATION")==0){
							set_rotation_state();
							get_screen_rotation();
							set_blocks();
							create_block_container(&container, blocks, number_of_blocks, 0,0,0,0,1);
							bar = render_bar(NULL, &container);
							write(lemon_write_fd, bar, strlen(bar));
							free(bar);
						}else
						if (strcmp(buff, "KEYBOARD")==0){
							set_keyboard_state();
							get_keyboard_block();
							set_blocks();
							create_block_container(&container, blocks, number_of_blocks, 0,0,0,0,1);
							bar = render_bar(NULL, &container);
							write(lemon_write_fd, bar, strlen(bar));
							free(bar);
						}
						else{
							system(buff);
						}
						free(buff);
					}
			}
			for (i = 0; i < external_comm_len;i++){
				if (FD_ISSET(external_comm_fd[i], &descriptors)){
					if ((buff = read_fd(external_comm_fd[i])) != NULL){
						parse_query(buff);
						set_blocks();
						create_block_container(&container, blocks, number_of_blocks, 0,0,0,0,1);
						bar = render_bar(NULL, &container);
						write(lemon_write_fd, bar, strlen(bar));
						free(bar);
						free(buff);
					}
				}
			}
		}
	}

	cleanup();
	kill(lemons, 15);
	for (i = 0; i < external_comm_len; i++){
		kill(external_comm_pid[i], 15);
	}
	if (socket_path != NULL) free(socket_path);
	free(external_comm_pid);
	free(external_comm_fd);
	return 0;	
}
