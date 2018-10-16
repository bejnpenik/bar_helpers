#ifndef _BAR_HELPERS_H
#define _BAR_HELPERS_H

typedef enum {
	NO_TOGGLE,
	TOGGLE
} attr_toggle_t;
typedef enum {
	LEFT_ALIGN,
	RIGHT_ALIGN,
	CENTER_ALIGN
} attr_align_t;


typedef enum {
	NO_ATTR = 1,
	UNDERLINE = 2,
	OVERLINE = 4
} attr_flag_t;

typedef enum {
	NO_CLICK = 1,
	LEFT_CLICK = 2,
	RIGHT_CLICK = 4,
	MIDDLE_CLICK = 8,
	WHEEL_UP = 16,
	WHEEL_DOWN = 32
} click_t;

typedef enum {
	DEFAULT_C = 1,
	BACKGROUND_C = 2,
	FOREGROUND_C = 4,
	UNDERLINE_C = 8
} color_t; 

typedef struct {
 	char *text;//display text
	color_t colors;//which colors are in char array
	char **color_codes;//codes for used colors
	click_t clicks;//which clicks
	char **commands;//what commands are for needed clicks
	unsigned int font_index;//special font for block text
	attr_flag_t attr; //block flags
	attr_align_t aligment;//block aligment
	int padding;
	int position;//position of block in aligment
} block_param_t;

typedef struct {
	char *block_string;
	attr_align_t aligment;
	int position;
} block_t;
typedef struct {
	int nbr_of_blocks;
	int left_blk_offset;
	int center_blk_offset_left;
	int center_blk_offset_right;
	int right_blk_offset;
	int padding;
	block_param_t **blocks;
} block_container_t;
static char *add_padding(char *, char *, int);
static char *create_command(char *, char *);
static char *create_click_tag(char *, int , char *, char *);
static char *create_clickable_block(char *, click_t , char **, char *);
static char *create_color_tag(char *, char , char *, char *);
static char *create_colorful_block(char *, color_t , char **, char *);
static char *add_attr_tag(char *, char , char *);
static char *remove_attr_tag(char *, char , char *);
static char *create_block_attributes(char *, attr_flag_t , char *);
static char *add_font_attr(char *, int , char *);
block_param_t *create_block_param_t(block_param_t *, char *, color_t, char **, click_t, char **,unsigned int, attr_flag_t, attr_align_t, int, int);
void update_block_text(block_param_t *, char *);
void update_block_colors(block_param_t *, char **, color_t);
void update_block_commands(block_param_t *, char **, click_t);
void update_block_alignment(block_param_t *, attr_align_t);
void update_block_attr(block_param_t *, attr_flag_t);
void update_block_padding(block_param_t *, int);
void update_block_position(block_param_t *, int);
void free_block_param_t(block_param_t *);
static block_t *create_block_t(block_t *, block_param_t *);
static void free_block_t(block_t *);
block_container_t *create_block_container(block_container_t *,block_param_t **,int,int, int, int, int, int);
static void sort_blocks(block_t **, const int);
static char *create_aligned_block(char *, attr_align_t, int, int, char *);
static char *add_external_padding(char *, int);
char *render_bar(char *, block_container_t *);
#endif 
