#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "bar_helpers.h"
// TODO: ALL FREED BLOCKS SHOULD BE SET TO NULL AND MALLOC SHOULD BE CHECKED!
static char *add_padding(char *dest, char *src, int padding){
	int text_len = strlen(src);
	char * dest_tmp = (char *)malloc(text_len + 2*padding + 1);
	for (int i=0;i<padding;i++){
		dest_tmp[i] = ' ';
	}
	dest_tmp[padding] = '\0';
	strncat(dest_tmp, src, text_len);
	for (int i=padding+text_len;i<2*padding+text_len;i++){
		dest_tmp[i] = ' ';
	}
	dest_tmp[text_len+2*padding]='\0';
	if (dest==NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, strlen(dest_tmp) + 1);
	free(dest_tmp);
	return dest;
	
	
}
static char *create_command(char *dest, char *text){
	size_t char_count = strlen(text);
	int nlen = char_count + 2;
	char *dest_tmp = (char *)malloc(nlen + 1);
	memset(dest_tmp,0, nlen+1);
	dest_tmp[0] = ':';
	for (int i=0; i<char_count; i++) dest_tmp[i+1] = text[i];
	dest_tmp[nlen-1] = ':';
	dest_tmp[nlen] = '\0';
	if (dest==NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, strlen(dest_tmp) + 1);
	free(dest_tmp);
	return dest;
}

static char *create_click_tag(char *dest, int nbr, char *command_text, char *text_to_wrap){
	const char CLICK_TAG[] = "%{A}";
	const int CLICK_CHAR_COUNT = 4; 
	const char CLOSE_TAG = '}';
	char *command = create_command(NULL, command_text);
	size_t char_count_command = strlen(command);
	size_t char_count_text = strlen(text_to_wrap);
	int nlen = char_count_command + char_count_text + 2*CLICK_CHAR_COUNT + 1;
	char *dest_tmp = (char *)malloc(nlen + 1);
	int position = 0;
	for (int i = 0; i < CLICK_CHAR_COUNT-1;i++){
		dest_tmp[position++] = CLICK_TAG[i];
	}
	dest_tmp[position++] = nbr + '0';
	for (int i = 0; i < char_count_command;i++){
		dest_tmp[position++] = command[i];
	}
	dest_tmp[position++] = CLOSE_TAG;
	for (int i = 0; i < char_count_text;i++){
		dest_tmp[position++] = text_to_wrap[i];
	}
	for (int i = 0; i < CLICK_CHAR_COUNT;i++){
		dest_tmp[position++] = CLICK_TAG[i];
	}
	dest_tmp[position] = '\0';
	free(command);
	if (dest==NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, nlen);
	free(dest_tmp);
	return dest;		
}	



static char *create_clickable_block(char *dest, click_t clicks, char **commands, char *text){
	
	char *tmp = NULL;
	char *dest_tmp;
	int tmp_len = strlen(text);
	dest_tmp = (char *) malloc(tmp_len+1);
	memcpy(dest_tmp, text, strlen(text)+1);
	int click_number[5] = {LEFT_CLICK, MIDDLE_CLICK, RIGHT_CLICK, WHEEL_UP, WHEEL_DOWN};
	if ((clicks & NO_CLICK) == NO_CLICK) goto end;
	for (int i=0; i < 5; i++){
		if ((clicks & click_number[i]) == click_number[i]){
			tmp = create_click_tag(NULL, i+1, commands[i], dest_tmp);
			tmp_len = strlen(tmp);
			dest_tmp = (char *)realloc(dest_tmp, tmp_len + 1);
			memcpy(dest_tmp, tmp, tmp_len+1);	
			free(tmp);
			tmp=NULL;
		}
	}
end:
	if (dest==NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, tmp_len+1);
	free(dest_tmp);
	return dest;
	
} 
static char *create_color_tag(char *dest, char color, char *color_code, char *text){	
	char c[2] = {color, '\0'};
	size_t r_size = strlen(color_code) + strlen(text) + 9;
	char *r = (char *) malloc(r_size + 1);
	memset(r, 0, r_size + 1);
	*r = '\0';
	strcat(r, "%{");
	strcat(r, c);
	strcat(r, color_code);
	strcat(r, "}");
	strcat(r, text);
	strcat(r, "%{");
	strcat(r, c);
	strcat(r, "-}");
	if (dest==NULL){
		return r;
	}
	memcpy(dest, r, r_size);
	free(r);
	return dest;
}
static char *create_colorful_block(char *dest, color_t colors, char **color_codes, char *text){
	
	char *tmp = NULL;
	char *dest_tmp;
	int tmp_len = strlen(text);
	dest_tmp = (char *) malloc(tmp_len+1);
	memcpy(dest_tmp, text, strlen(text)+1);
	int color[3] = {FOREGROUND_C, BACKGROUND_C, UNDERLINE_C};
	char color_tag[3] = {'F', 'B', 'U'};
	if ((colors & DEFAULT_C) == DEFAULT_C) goto end;
	for (int i=0; i<3;i++){
		if ((colors & color[i]) == color[i]){
			
			tmp = create_color_tag(NULL, color_tag[i], color_codes[i], dest_tmp);
			tmp_len = strlen(tmp);
			dest_tmp = (char *)realloc(dest_tmp, tmp_len + 1);
			memcpy(dest_tmp, tmp, tmp_len+1);	
			free(tmp);
			tmp=NULL;
		}
	}
end:
	if (dest==NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, tmp_len+1);
	free(dest_tmp);
	return dest;
}
static char *add_attr_tag(char *dest, char attr, char *text){	
	char attr_add[6] = {'%', '{','+', attr, '}', '\0'};
	//char attr_remove[5] = {'%', '{','-', attr, '}', '\0'};
	size_t r_size = strlen(attr_add) + strlen(text) + 1;
	char *r = (char *) malloc(r_size + 1);
	memset(r, 0, r_size + 1);
	*r = '\0';
	strcat(r, attr_add);
	strcat(r, text);
	if (dest==NULL){
		return r;
	}
	memcpy(dest, r, r_size);
	free(r);
	return dest;
}

static char *remove_attr_tag(char *dest, char attr, char *text){	
	//char attr_add[6] = {'%', '{','+', attr, '}', '\0'};
	char attr_remove[6] = {'%', '{','-', attr, '}', '\0'};
	size_t r_size = strlen(attr_remove) + strlen(text) + 1;
	char *r = (char *) malloc(r_size + 1);
	memset(r, 0, r_size + 1);
	*r = '\0';
	strcat(r, attr_remove);
	strcat(r, text);
	if (dest==NULL){
		return r;
	}
	memcpy(dest, r, r_size);
	free(r);
	return dest;
}

static char *create_block_attributes(char *dest, attr_flag_t attrs, char *text){
	char *dest_tmp = NULL;
	char *tmp = NULL;	
	if ((attrs & NO_ATTR) == NO_ATTR){
		tmp = remove_attr_tag(NULL, 'u', text);
		dest_tmp = remove_attr_tag(NULL, 'o', tmp);
		free(tmp);
		if (dest==NULL){
			return dest_tmp;
		}
		memcpy(dest, dest_tmp, strlen(dest_tmp)+1);
		free(dest_tmp);
		return dest;
		
		
	}
	int tmp_len = strlen(text);
	dest_tmp = (char *) malloc(tmp_len+1);
	memcpy(dest_tmp, text, strlen(text)+1);
	if ((attrs & UNDERLINE) == UNDERLINE){
		tmp = add_attr_tag(NULL, 'u', dest_tmp);
		tmp_len = strlen(tmp);
		dest_tmp = (char *)realloc(dest_tmp, tmp_len + 1);
		memcpy(dest_tmp, tmp, strlen(tmp)+1);
		free(tmp);
		tmp=NULL;
	}
	if ((attrs & OVERLINE) == OVERLINE){
		tmp = add_attr_tag(NULL, 'o', dest_tmp);
		tmp_len = strlen(tmp);
		dest_tmp = (char *)realloc(dest_tmp, tmp_len + 1);
		memcpy(dest_tmp, tmp, strlen(tmp)+1);
		free(tmp);
		tmp=NULL;
	}

	if (dest==NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, strlen(dest_tmp)+1);
	free(dest_tmp);
	return dest;
}

static char *add_font_attr(char *dest, int nbr, char *text){
	size_t r_size = strlen(text) + 10;
	char *r = (char *) malloc(r_size + 1);
	char cnbr[2];
	cnbr[0] = nbr + '0';
	cnbr[1] = '\0';
	memset(r, 0, r_size+1);
	*r = '\0';
	strcat(r, "%{T");
	strcat(r, cnbr);
	strcat(r, "}");
	strcat(r, text);
	strcat(r, "%{T-}");
	if (dest==NULL){
		return r;
	}
	memcpy(dest, r, r_size);
	free(r);
	return dest;
}
block_param_t *create_block_param_t(block_param_t *dest, 
									char *text,//display text
									color_t colors,//which colors are in char array
									char **color_codes,//codes for used colors
									click_t clicks,//which clicks
									char **commands,//what commands are for needed clicks
									unsigned int font_index,//special font for block text
									attr_flag_t attr, //block flags
									attr_align_t aligment,//block aligment
									int padding,
									int position)//position of block in aligment)
{
	block_param_t *dest_tmp = (block_param_t *)malloc(sizeof(block_param_t));
	int text_len = strlen(text);
	dest_tmp->text = (char *) malloc(text_len+1);
	memset(dest_tmp->text, 0, text_len+1);
	strncpy(dest_tmp->text, text, text_len);
	dest_tmp->colors = colors;
	int tmp_len;
	dest_tmp->color_codes = (char**)malloc(3*sizeof(char*));
	for (int i=0;i<3;i++) {
		dest_tmp->color_codes[i] = NULL;
		if (color_codes[i] != NULL){
			tmp_len = strlen(color_codes[i]);
			dest_tmp->color_codes[i] = (char*)malloc(tmp_len+1);
			memset(dest_tmp->color_codes[i], 0, tmp_len + 1);
			strncpy(dest_tmp->color_codes[i], color_codes[i], tmp_len);
		}
		
	}	
	dest_tmp->clicks = clicks;
	dest_tmp->commands = (char**)malloc(5*sizeof(char*));
	for (int i=0;i<5;i++) {
		dest_tmp->commands[i] = NULL;
		if (commands[i] != NULL){
			tmp_len = strlen(commands[i]);
			dest_tmp->commands[i] = (char*)malloc(tmp_len+1);
			memset(dest_tmp->commands[i], 0, tmp_len + 1);
			strncpy(dest_tmp->commands[i], commands[i], tmp_len);
		}
		
	}
	dest_tmp->font_index = font_index;
	dest_tmp->attr = attr;
	dest_tmp->aligment = aligment;
	dest_tmp->padding = padding;
	dest_tmp->position = position;
	if (dest == NULL)
		return dest_tmp;
	if (dest->text != NULL){
		free(dest->text);
	}
	if (dest->color_codes != NULL){
		for (int i=0; i<3; i++)
			if (dest->color_codes[i] != NULL) free(dest->color_codes[i]);
		free(dest->color_codes);
	}
	if (dest->commands != NULL){ 
		for (int i=0; i<5; i++)
			if (dest->commands[i] != NULL) free(dest->commands[i]);
		free(dest->commands);
	}
	memcpy(dest, dest_tmp, sizeof(block_param_t));
	free(dest_tmp);
	return dest;
}
void free_block_param_t(block_param_t *block){
	free(block->text);
	for (int i = 0; i < 3; i++) block->color_codes[i] != NULL ? free(block->color_codes[i]):NULL;
	free(block->color_codes);
	for (int i = 0; i < 5; i++) block->commands[i] != NULL ? free(block->commands[i]):NULL;
	free(block->commands);
	free(block);
}
static block_t *create_block_t(block_t *dest, block_param_t *block_params){
	block_t *dest_tmp = (block_t*) malloc(sizeof(block_t));
	dest_tmp -> position = block_params->position;
	dest_tmp -> aligment = block_params->aligment;
	char *text_padding = add_padding(NULL, block_params -> text, block_params->padding);
	char *tmp = NULL;
	tmp = strdup(text_padding);
	if (block_params -> font_index > 0 && block_params -> font_index < 6){
		free(tmp);
		tmp = add_font_attr(NULL, block_params -> font_index, text_padding);
	}
	char *click_block = create_clickable_block(NULL, block_params->clicks, block_params->commands, tmp);
	char *color_block = create_colorful_block(NULL, block_params->colors, block_params->color_codes, click_block);
	dest_tmp -> block_string = create_block_attributes(NULL, block_params->attr, color_block);
	free(text_padding);
	free(tmp);	
	free(click_block);
	free(color_block);
	if (dest == NULL)
		return dest_tmp;
	memcpy(dest, dest_tmp, sizeof(block_t));
	free(dest_tmp);
	return dest;
}
block_container_t *create_block_container(block_container_t *dest, 
											block_param_t **blocks, 
											int nbr_of_blocks, 
											int left_block_offset,
											int right_block_offset,
											int center_block_offset_left,
											int center_block_offset_right,
											int padding){
	block_container_t *dest_tmp = (block_container_t*)malloc(sizeof(block_container_t));
	/*int nbr_of_blocks;
	int left_blk_offset;
	int center_blk_offset_left;
	int center_blk_offset_right;
	int right_blk_offset;
	int padding;
	block_param_t **blocks;*/
	dest_tmp -> blocks = blocks;
	dest_tmp -> nbr_of_blocks = nbr_of_blocks;
	dest_tmp -> left_blk_offset = left_block_offset;
	dest_tmp -> right_blk_offset = right_block_offset;
	dest_tmp -> center_blk_offset_left = center_block_offset_left;
	dest_tmp -> center_blk_offset_right = center_block_offset_right;
	dest_tmp -> padding = padding;
	if (dest == NULL)
		return dest_tmp;
	memcpy(dest, dest_tmp, sizeof(block_container_t));
	free(dest_tmp);
	return dest;
	

}
static void free_block_t(block_t *block){
	free(block -> block_string);
	free(block);
}

static void sort_blocks(block_t **blocks, const int nbr_of_blocks){
	//buble sort http://www.zentut.com/c-tutorial/c-bubble-sort/
	block_t *tmp;
	for (int i = 0; i < nbr_of_blocks; i++){
			for (int j = 0; j < (nbr_of_blocks - (i + 1)); j++){
				if (blocks[j]->position>blocks[j+1]->position){
					tmp = blocks[j];
					blocks[j] = blocks[j+1];
					blocks[j+1] = tmp;
				}
			}
		}
	
}
static char *create_aligned_block(char *dest, attr_align_t alignment, int left_offset, int right_offset, char *text){
	//TODO:OFFSET
	//left_offset is before block text
	//right_offset is after block
	int align_attr_len = 4;//strlen("%{u}")
	int block_len = strlen(text) + align_attr_len;
	char *dest_tmp = (char *)malloc(block_len+1);
	memset(dest_tmp, 0, block_len+1);
	switch(alignment){
		case LEFT_ALIGN:
			strncat(dest_tmp, "%{l}", align_attr_len);
			strcat(dest_tmp, text);
			break;
		case CENTER_ALIGN:
			strncat(dest_tmp, "%{c}", align_attr_len);
			strcat(dest_tmp, text);
			break;
		case RIGHT_ALIGN:
			strncat(dest_tmp, "%{r}", align_attr_len);
			strcat(dest_tmp, text);
			break;
	}
	if (dest == NULL)
		return dest_tmp;
	memcpy(dest, dest_tmp, block_len);
	free(dest_tmp);
	return dest;
}
static char *add_external_padding(char *dest, int padding){
	char *dest_tmp;
	char remove_attrs[] = "%{-u}%{-o}";
	dest_tmp = (char *)malloc(padding + 11);
	memset(dest_tmp, 0, 11+padding);
	dest_tmp[0] = '\0';
	strncat(dest_tmp, remove_attrs, 10);
	
	for (int i = 10; i < 10 + padding; i++){
		//printf("%d\n", i);
		dest_tmp[i] = ' ';
	}
	dest_tmp[10 + padding] = '\0';
	//printf("%s%d\n", dest_tmp, strlen(dest_tmp));
	if (dest == NULL){
		return dest_tmp;
	}
	memcpy(dest, dest_tmp, padding + 11);
	free(dest_tmp);
	return dest;
	
}
char *render_bar(char *dest, block_container_t * block_container){
	int nbr_of_blocks = block_container -> nbr_of_blocks;
	block_param_t **blocks = block_container -> blocks;
	int loffset = block_container -> left_blk_offset;
	int roffset = block_container -> right_blk_offset;
	int coffsetl = block_container -> center_blk_offset_left;
	int coffsetr = block_container -> center_blk_offset_left;
	int padding  = block_container -> padding;
	int nbr_of_left_blocks = 0;
	int left_len = 0;
	int nbr_of_center_blocks = 0;
	int center_len = 0;	
	int nbr_of_right_blocks = 0;
	int right_len = 0;
	char *tmp = add_external_padding(NULL, padding);
	block_t **left_blocks = (block_t **) malloc(nbr_of_blocks*sizeof(block_t*));
	block_t **center_blocks = (block_t **) malloc(nbr_of_blocks*sizeof(block_t*));
	block_t **right_blocks = (block_t **) malloc(nbr_of_blocks*sizeof(block_t*));
	for (int i = 0; i < nbr_of_blocks; i++){
		if (blocks[i]->aligment == LEFT_ALIGN){
			left_blocks[nbr_of_left_blocks] = create_block_t(NULL, blocks[i]);
			left_len += strlen(left_blocks[nbr_of_left_blocks]->block_string) + strlen(tmp);
			nbr_of_left_blocks++;
		}else if(blocks[i]->aligment == CENTER_ALIGN){
			center_blocks[nbr_of_center_blocks] = create_block_t(NULL, blocks[i]);
			center_len += strlen(center_blocks[nbr_of_center_blocks]->block_string) + strlen(tmp);
			nbr_of_center_blocks++;
		}else if (blocks[i]->aligment == RIGHT_ALIGN){
			right_blocks[nbr_of_right_blocks] = create_block_t(NULL, blocks[i]);
			right_len += strlen(right_blocks[nbr_of_right_blocks]->block_string) + strlen(tmp);
			nbr_of_right_blocks++;
		}else{
		}
	}
	char *left_block = (char *)malloc(left_len + 1);
	char *left_block_aligned = NULL;
	char *center_block = (char *)malloc(center_len + 1);
	char *center_block_aligned = NULL;
	char *right_block = (char *)malloc(right_len + 1);
	char *right_block_aligned = NULL;
	
	//printf("1%s\n", tmp);
	//int curr_pos = 0;
	left_block[0] = '\0';
	if (nbr_of_left_blocks>0){
		
		sort_blocks(left_blocks, nbr_of_left_blocks);
		
		for (int i = 0; i < nbr_of_left_blocks; i++){
			//tmp = add_external_padding(NULL, padding);
			//strcat(left_block, tmp);
			strcat(left_block, left_blocks[i] -> block_string);
			strcat(left_block, tmp);
			/*free(left_blocks[i] -> block_string);
			free(left_blocks[i]);*/
			//free(tmp);
			//tmp = NULL;
			free_block_t(left_blocks[i]);
		}
		left_block_aligned = create_aligned_block(NULL, LEFT_ALIGN, 0,loffset, left_block);
	}
	center_block[0] = '\0';
	if (nbr_of_center_blocks>0){
		sort_blocks(center_blocks, nbr_of_center_blocks);
		
		for (int i = 0; i < nbr_of_center_blocks; i++){
			//tmp = add_padding(NULL, center_blocks[i]->block_string, padding);
			//strcat(center_block, tmp);
			strcat(center_block, center_blocks[i] -> block_string);
			strcat(center_block, tmp);
			/*free(center_blocks[i] -> block_string);
			free(center_blocks[i]);*/
			//free(tmp);
			//tmp = NULL;
			free_block_t(center_blocks[i]);
		}
		center_block_aligned = create_aligned_block(NULL, CENTER_ALIGN, coffsetl , coffsetr, center_block);
		
	}
	right_block[0] = '\0';
	if (nbr_of_right_blocks>0){
		sort_blocks(right_blocks, nbr_of_right_blocks);
		
		for (int i = 0; i < nbr_of_right_blocks; i++){
			//tmp = add_padding(NULL, right_blocks[i]->block_string, padding);
			//strcat(right_block, tmp);
			strcat(right_block, right_blocks[i]->block_string);
			strcat(right_block, tmp);
			/*free(right_blocks[i] -> block_string);
			free(right_blocks[i]);*/
			//free(tmp);
			//tmp = NULL;
			free_block_t(right_blocks[i]);
		}
		right_block_aligned = create_aligned_block(NULL, RIGHT_ALIGN, roffset,0, right_block);
	}
	if (left_block_aligned == NULL){
		left_block_aligned = (char*)malloc(sizeof(char));
		left_block_aligned[0] = '\0';
	}
	if (center_block_aligned == NULL){
		center_block_aligned = (char*)malloc(sizeof(char));
		center_block_aligned[0] = '\0';
	}
	if (right_block_aligned == NULL){
		right_block_aligned = (char*)malloc(sizeof(char));
		right_block_aligned[0] = '\0';
	}
	int bar_block_len = strlen(left_block_aligned) + strlen(center_block_aligned) + strlen(right_block_aligned);
	char *dest_tmp = (char *)malloc(bar_block_len + 1);
	memset(dest_tmp, 0, bar_block_len + 1);
	dest_tmp[0] = '\0';
	strncat(dest_tmp, left_block_aligned, strlen(left_block_aligned));
	strncat(dest_tmp, center_block_aligned, strlen(center_block_aligned));
	strncat(dest_tmp, right_block_aligned, strlen(right_block_aligned));
	free(left_blocks);
	free(left_block);
	free(left_block_aligned);
	free(center_blocks);
	free(center_block);
	free(center_block_aligned);
	free(right_blocks);
	free(right_block);
	free(right_block_aligned);	
	free(tmp);
	if (dest == NULL)
		return dest_tmp;
	memcpy(dest, dest_tmp, bar_block_len);
	free(dest_tmp);
	return dest;
}
int maind(){
	//char text[] = COMMAND(PROBA);
	char *commands[5] = {"echo 1", "echo 2", "echo 3", "echo 4", "echo 5"};
	int clicks = LEFT_CLICK | RIGHT_CLICK | MIDDLE_CLICK | WHEEL_UP | WHEEL_DOWN;
	char *paste_text = create_clickable_block(NULL, clicks, commands, "TEST CLICK");
	char *color_codes[3] = {"#616161", "#1b1b1b", "#665c54"};
	int colors = BACKGROUND_C | FOREGROUND_C | UNDERLINE_C;
	char *colorful = create_colorful_block(NULL, colors, color_codes, paste_text);
	int attrs = UNDERLINE | OVERLINE;
	//char *test = add_attr_tag(NULL, 'u', colorful);
	char *atrribute = create_block_attributes(NULL, attrs, colorful);
	//char *test3 = create_color_tag(NULL, 'B', "#1b1b1b",test2 );
	printf("%s\n", atrribute, strlen(atrribute));
	free(paste_text);
	free(atrribute);
	//free(test2);
	free(colorful);
}
/*t(block_param_t *dest, 
									char *text,//display text
									color_t colors,//which colors are in char array
									char **color_codes,//codes for used colors
									click_t clicks,//which clicks
									char **commands,//what commands are for needed clicks
									unsigned int font_index,//special font for block text
									attr_flag_t attr, //block flags
									attr_align_t aligment,//block aligment
									int padding,
									int position)*/
/*block_param_t *test_func_create_block_param_t(char *text, int position, int aligment){
	char *COMMANDS[5] = {"echo 1", "echo 2", "echo 3", "echo 4", "echo 5"};
	int clicks = 	 LEFT_CLICK | RIGHT_CLICK | MIDDLE_CLICK | WHEEL_UP | WHEEL_DOWN;
	//int clicks = NO_CLICK;
	char *color_codes[3] = {"#616161", "#1b1b1b", "#665c54"};
	//char *color_codes[3] = {"#616161", "#1b1b1b", NULL};
	int padding = 2;
	int colors = BACKGROUND_C | FOREGROUND_C | UNDERLINE_C;
	//int colors = DEFAULT_C;
	int attrs = UNDERLINE | OVERLINE;
	int findex = 0;
	
	return create_block_param_t(NULL, text, colors, color_codes, clicks, COMMANDS, findex, attrs, aligment, padding, position);
	
}*/

