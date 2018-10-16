#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
char brightness_command[] = "/sys/class/backlight/intel_backlight/brightness";
char actual_brightness_command[] = "/sys/class/backlight/intel_backlight/actual_brightness";
char max_brightness_command[] = "/sys/class/backlight/intel_backlight/max_brightness";
char DATA[256] = {0};


int actual_brightness_level_new;
int actual_brightness_level_old;
int brightness_level_new;
int brightness_level_old;
int max_brightness_level;


int actual_brightness_level_change(){
	if (actual_brightness_level_new == actual_brightness_level_old) return 0;
	actual_brightness_level_old = actual_brightness_level_new;
	return 1;
}

int brightness_level_change(){
	if (brightness_level_new == brightness_level_old) return 0;
	brightness_level_old = brightness_level_new;
	return 1;
}

int external_monitor_check(){
	if (brightness_level_new != actual_brightness_level_new && actual_brightness_level_new == 0) return 1;
	return 0;
}
void info(){
	int level;
	if (external_monitor_check()){
		if (brightness_level_change()){
			level = brightness_level_new*100/max_brightness_level;
			printf("LUM;%d",  level);
			fflush(stdout);
		}
	}else{
		if (actual_brightness_level_change())
			printf("LUM;%d", actual_brightness_level_new*100/max_brightness_level);
			fflush(stdout);
	}
}

FILE* bdopen(char const *cname, char leave_open){
	FILE *fin = fopen(cname, "r");
	setvbuf(fin, NULL, _IONBF, 0);
	fgets(DATA, 256, fin);
	if(leave_open==0){
		fclose(fin);
		return NULL;
	}
	else return fin;
}
int main (){
	FILE *fmaxb = bdopen(max_brightness_command, 0);
	max_brightness_level = atoi(DATA);
	
	FILE *fab = bdopen(actual_brightness_command, 1);
	actual_brightness_level_new = actual_brightness_level_old = atoi(DATA);
	FILE *fb = bdopen(brightness_command, 1);
	brightness_level_new = brightness_level_old = atoi(DATA);
	if (external_monitor_check()){
		printf("LUM;%d", brightness_level_new*100/max_brightness_level);
		fflush(stdout);
	}
	else {
		printf("LUM;%d", actual_brightness_level_new*100/max_brightness_level);
		fflush(stdout);
	}
	while (1){
		fseek(fab, 0, SEEK_SET);
		fgets(DATA, 256, fab);
		actual_brightness_level_new = atoi(DATA);
		fseek(fb, 0, SEEK_SET);
		fgets(DATA, 256, fb);
		brightness_level_new = atoi(DATA);
		info();
		sleep(1);
			
		
	}
	
}

