/**
 * SCT Sciences Creamy Text Editor (c) Jeron Lau RAHS Computer Science Club.
 * The Unlicense
**/

#include <stdio.h> // For using printf()
#include <string.h> // For using strlen()
#include <ncurses.h> // For getting input, etc.
#include <stdlib.h> // For exit()

#include "sct.h"

#define UNTITLED "UN-TITLED"

static textlist_t* init_textlist(void) {
	textlist_t* rtn = malloc(sizeof(textlist_t));
	rtn->prev = NULL;
	memset(rtn->text, '\0', 81);
	rtn->next = NULL;
	rtn->tabs = 0;
	return rtn;
}

void textlist_insert(textlist_t* afterwhat) {
	textlist_t* new_next = afterwhat->next;
	textlist_t* new_item = malloc(sizeof(textlist_t));

	// Sandwich
	afterwhat->next = new_item;
	if(new_next) new_next->prev = new_item;

	// Set item
	new_item->prev = afterwhat;
	new_item->next = new_next;

	memset(new_item->text, '\0', 81);
	new_item->tabs = 0;
}

static textlist_t* textlist_delete(textlist_t* deletewhat) {
	textlist_t* next_item = deletewhat->next;

	next_item->prev = NULL;

	free(deletewhat);

	return next_item;
}

static textlist_t* sct_nextline(textlist_t* line, textlist_t** text, int8_t* y,
	uint32_t* sln)
{
	if(*y == 9) {
		*sln = *sln + 1;
		*text = (*text)->next;
		return line->next;
	}
	*y = *y + 1;
	return line->next;
}

static textlist_t* sct_prevline(textlist_t* line, textlist_t** text, int8_t* y,
	uint32_t* sln)
{
	if(*y == 0) {
		if(*sln) {
			*sln = *sln - 1;
			*text = (*text)->prev;
			return line->prev;
		}
		return line;
	}
	*y = *y - 1;
	return line->prev;
}

static void sct_exit(uint8_t* running, const char* message) {
	*running = 0;
	endwin();
}

static void sct_draw(WINDOW* w, textlist_t* text, textlist_t* line,
	int8_t* x, int8_t* y, uint32_t sln)
{
	int i;
	uint8_t infile = 1;

	for(i = 0; i < 10; i++) {
		if(infile) {
			for(int j = 0; j < text->tabs; j++) {
				mvprintw(i + 1, (j+1) * 8, "        ");
			}
			mvprintw(i + 1, 0, "%5d | ", sln + i + 1);
			mvprintw(i + 1, 8 + text->tabs * 8, "%s\n", text->text);
			if(text->next) {
				text = text->next;
			}else{
				infile = 0;
			}
		}else{
			mvprintw(i + 1, 0, "      | ");
			mvprintw(i + 1, 8, "\n");
		}
	}
	wmove(w, 1 + *y, 8 + *x + (line->tabs * 8));
	chgat(1, A_REVERSE, 0, NULL);
	refresh();
}

/*static void sct_rendermultiselect(WINDOW* w, textlist_t text, int8_t* x, int8_t* y,
	int8_t* lx, int8_t* ly)
{
	wmove(w, 1 + *y, *x + (line->tabs * 8));
	chgat(1, A_REVERSE, 0, NULL);
	refresh();
}*/

static void sct_renderlineselect(WINDOW* w, textlist_t* line, int8_t x,
	int8_t y, int8_t lx)
{
	wmove(w, 1 + y, x + (line->tabs * 8));
	chgat(lx - x, A_REVERSE, 0, NULL);
	refresh();
}

void sct_insert(WINDOW* w, textlist_t* text, textlist_t* line,
	int8_t* x, int8_t* y, char c, uint32_t sln)
{
	uint8_t max = 79 - (line->tabs * 8);
	if(*x > max) *x = max;
	if(strlen(line->text) >= max) return;
	memmove(line->text + *x + 1, line->text + *x, strlen(line->text + *x) + 1);
	line->text[*x] = c;
	*x = *x + 1;
	sct_draw(w, text, line, x, y, sln);
}

static int8_t sct_mouseinputx(textlist_t* line, int8_t x) {
	int rx = x - ((line->tabs + 1)* 8);

	if(rx < 0) rx = 0;
	if(rx > strlen(line->text)) rx = strlen(line->text);
	if(x > 79) x = 79;
	return rx;
}

static int8_t sct_mouseinputy(int8_t y) {
	int ry = y - 1;

	if(ry < 0) ry = 0;
	return ry;
}

static textlist_t* sct_cursorup(WINDOW* w, textlist_t** text, textlist_t* line,
	int8_t* x, int8_t* y, uint32_t* sln)
{
	if(line->prev) {
		line = sct_prevline(line, text, y, sln);
		*x = strlen(line->text);
		sct_draw(w, *text, line, x, y, *sln);
	}
	return line;
}

static textlist_t* sct_cursordn(WINDOW* w, textlist_t** text, textlist_t* line,
	int8_t* x, int8_t* y, uint32_t* sln, uint8_t end)
{
	if(line->next) {
		line = sct_nextline(line, text, y, sln);
		*x = end ? strlen(line->text) : 0;
		sct_draw(w, *text, line, x, y, *sln);
	}
	return line;
}

static textlist_t* sct_backspace(WINDOW* w, textlist_t** text,
	textlist_t* line, int8_t* x, int8_t* y, uint32_t* sln)
{
	textlist_t* prev_item = line->prev;
	textlist_t* next_item = line->next;

	if(next_item) next_item->prev = prev_item;
	prev_item->next = next_item;

	free(line);

	// Scroll Up if Needed
	if(*sln) {
		int end;
		textlist_t* temp = *text;
		for(int i = 0; i < 10; i++) {
			temp = temp->next;
			if(temp == NULL && (*text)->prev) {
				*sln = *sln - 1;
				*y = *y + 1;
				*text = (*text)->prev;
				break;
			}
		}
	}

	if(next_item) {
		return next_item;
	}else{
		sct_cursorup(w, text, line, x, y, sln);
		return prev_item;
	}
}

static void sct_title(int32_t w, int32_t h, const char* f, uint8_t s) {
	mvprintw(0,0,"Science's Creamy Text Editor - SCT %03dx%03d %44s",w,h,f);
	if(!s) mvprintw(0,86 - strlen(f), "*");
}

static void sct_notify(const char* message) {
	mvprintw(11, 0, "|-----| %77s |", message);
}

static uint8_t sct_popup(WINDOW* w, const char* message, char input[]) {
	uint8_t cancel = 0;

	memset(input, '\0', strlen(input));
	mvprintw(2,14,"                                                      ");
	mvprintw(3,14," ____________________________________________________ ");
	mvprintw(4,14," |%50s| ", message);
	mvprintw(5,14," |__________________________________________________| ");
	mvprintw(6,14," | >:                                               | ");
	mvprintw(7,14," |__________________________________________________| ");
	mvprintw(8,14,"                                                      ");
	mvprintw(6,20,"%s", input);
	while(1) {
		int in = getch();
		const char* key = keyname(in);

		if(in == ERR) continue;
		if(strcmp(key, "KEY_BACKSPACE") == 0) {
			input[strlen(input) - 1] = '\0';
		}else if(strcmp(key, "^J") == 0) {
			cancel = 0;
			break;
		}else if(strlen(key) == 1) {
			if(strlen(input) > 44) continue;
			input[strlen(input)] = key[0];
			input[strlen(input) + 1] = '\0';
		}else{
			sct_notify("Canceled.");
			cancel = 1;
			memcpy(input, UNTITLED, strlen(UNTITLED) + 1);
			break;
		}
		mvprintw(6, 20, "%s ", input);
	}
	return cancel;
}

// Main function.
int main(int argc, char *argv[]) {
	// Variable to store character name in.
	textlist_t* text = init_textlist();
	textlist_t* line = text;
	uint8_t running = 1;
	WINDOW *w = initscr();
	int8_t cursorx = 0, cursory = 0;
	int8_t selectx = 0, selecty = 0;
	uint8_t mouseHeldDown = 0;
	char filename[1024];
	uint32_t sln = 0; // Starting Line Number
	int16_t width = getmaxx(w);
	int16_t height = getmaxy(w);
	uint8_t saved = 1;

	memset(filename, 0, 1024);
	if(argc == 2) {
		memcpy(filename, argv[1], strlen(argv[1]));
		sct_file_load(filename, text);
	}else{
		memcpy(filename, UNTITLED, strlen(UNTITLED));
	}

	cbreak();
	noecho();
	nodelay(w, TRUE);
	keypad(w, TRUE);
	meta(w, TRUE);
	nodelay(w, TRUE);
	curs_set(FALSE);
	raw();
	mousemask(ALL_MOUSE_EVENTS, NULL);

	sct_title(width, height, filename, saved);
	sct_draw(w, text, line, &cursorx, &cursory, sln);
	sct_notify("Started Science's Creamy Text Editor!");
	while(running) {
		int32_t chr = getch();
		if(chr != ERR) {
			const char* name = keyname(chr);

		SCT_DETECT:;
			// Close
			if(strcmp(name, "^Q") == 0) {
				sct_exit(&running, "EXITING ON QUIT");
			}
			else if(strcmp(name, "^W") == 0) {
				sct_exit(&running, "EXITING ON FILE CLOSE");
			}
			// Clipboard
			else if(strcmp(name, "^C") == 0) {
				sct_notify("Copy.");
			}
			else if(strcmp(name, "^V") == 0) {
				sct_notify("Paste.");
			}
			else if(strcmp(name, "^X") == 0) {
				sct_notify("Cut.");
			}
			// Save
			else if(strcmp(name, "^S")==0 || strcmp(name, "^E")==0){
				if(strcmp(filename, UNTITLED) == 0) {
					if(sct_popup(w, "Save As...", filename))
					{
						sct_draw(w, text, line, &cursorx,
							&cursory, sln);
						continue;
					}
					// Redraw screen
					sct_draw(w, text, line, &cursorx,
						&cursory, sln);
				}
				sct_file_save(filename, text);
				saved = 1;
				sct_title(width,height,filename,saved);
				sct_notify("Saved file.");
				if(strcmp(name, "^E") == 0)
					sct_exit(&running, "SAVE & QUIT");
			}
			else if(strcmp(name, "^O") == 0) {
				if(sct_popup(w, "Open What File?", filename)) {
					sct_draw(w, text, line, &cursorx,
						&cursory, sln);
					continue;
				}
				while(text->prev != NULL)
					text = text->prev;
				if(sct_file_load(filename, text)) {
					sct_notify("Failed to open file.");
				}else{
					sct_notify("Successfully loaded file.");
				}
				saved = 1, sln = 0, cursorx = 0, cursory = 0,
					selectx = 0, selecty = 0, line = text;
				sct_title(width, height, filename, saved);
				// Redraw screen
				sct_draw(w, text, line, &cursorx, &cursory, sln);
			}
			// Functions
			else if(strcmp(name, "KEY_BACKSPACE") == 0) {
				cursorx--;
				if(cursorx < 0) {
					if(line->tabs > 0) {
						line->tabs--;
						cursorx = 0;
					}else if(line->prev != NULL) {
						line = sct_backspace(w,
							&text, line, &cursorx,
							&cursory, &sln);
						cursorx = strlen(line->text);
					}else{
						cursorx = 0;
					}
				}else{
					line->text[cursorx] = '\0';
					if(line->text[cursorx + 1])
						memmove(line->text + cursorx,
							line->text+cursorx+1,
							strlen(line->text +
								cursorx + 1)+1);
				}
				sct_draw(w, text, line, &cursorx, &cursory, sln);
				// Changed
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
			}
			else if(strcmp(name, "KEY_DC") == 0) {
				sct_exit(&running, "Delete");
//				line = sct_backspace(line->next);
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
			}
			else if(strcmp(name, "^D") == 0) {
				if(line->prev != NULL) {
					// Delete non-first line
					line = sct_backspace(w, &text,
						line, &cursorx, &cursory, &sln);
//					if(line->next)
//						line = line->next;
//					else
//						sct_cursorup();
					cursorx = strlen(line->text);
				}else if(line->next != NULL) {
					// Delete 1st line
					text = textlist_delete(line);
					line = text;
					cursorx = 0;
				}else{
					// Delete last line left
					memset(text->text, '\0', 81);
					cursorx = 0;
				}
				sct_draw(w, text, line, &cursorx, &cursory, sln);
				// Changed
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
			}
			else if(strcmp(name, "^I") == 0) {
				// Tab
				line->tabs++;
				if(line->tabs > 8) line->tabs = 8;
				sct_draw(w, text, line, &cursorx, &cursory, sln);
				// Input
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
			}
			else if(strcmp(name, "KEY_BTAB") == 0) {
				// Shift - Tab
				line->tabs--;
				if(line->tabs < 0) line->tabs = 0;
				sct_draw(w, text, line, &cursorx, &cursory, sln);
				// Input
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
			}
			else if(strcmp(name, "^J") == 0) {
				// Newline
				textlist_insert(line);
				line = sct_nextline(line, &text, &cursory, &sln);
				cursorx = 0;
				sct_draw(w, text, line, &cursorx, &cursory, sln);
				// Input
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
			}
			else if(strcmp(name, "^[") == 0) {
				int32_t chrtr;
				const char* which = NULL;

				sct_notify("w or s - save, q - quit, e - save &"
					" quit, o - open, d - delete line");
				while(1) {
					chrtr = getch();
					which = keyname(chrtr);
					if(strlen(which) == 1 && chrtr != ERR)
						break;
				}
				if(which[0] == 'w' || which[0] == 's') {
					name = "^S";
					goto SCT_DETECT;
				}else if(which[0] == 'q') {
					name = "^Q";
					goto SCT_DETECT;
				}else if(which[0] == 'e') {
					name = "^E";
					goto SCT_DETECT;
				}else if(which[0] == 'o') {
					name = "^O";
					goto SCT_DETECT;
				}else if(which[0] == '^') {
					continue;
				}else if(which[0] == 'd') {
					name = "^D";
					goto SCT_DETECT;
				}else{
					sct_notify("unknown function.");
				}
				// Alt + other
			}
			else if(strcmp(name, "KEY_UP") == 0) {
				line = sct_cursorup(w, &text, line, &cursorx,
					&cursory, &sln);
			}
			else if(strcmp(name, "KEY_DOWN") == 0) {
				line = sct_cursordn(w, &text, line, &cursorx,
					&cursory, &sln, 1);
			}
			else if(strcmp(name, "KEY_RIGHT") == 0) {
				if(cursorx >= strlen(line->text)) {
					line = sct_cursordn(w, &text, line,
						&cursorx, &cursory, &sln, 0);
				}else{
					cursorx++;
					sct_draw(w, text, line, &cursorx,
						&cursory, sln);
				}
			}
			else if(strcmp(name, "KEY_LEFT") == 0) {
				if(cursorx <= 0) {
					line = sct_cursorup(w, &text, line,
						&cursorx, &cursory, &sln);
				}else{
					cursorx--;
					sct_draw(w, text, line, &cursorx,
						&cursory, sln);
				}
			}
			else if(strcmp(name, "KEY_PPAGE") == 0) {
				// Page UP
			}
			else if(strcmp(name, "KEY_NPAGE") == 0) {
				// Page DN
			}
			else if(strcmp(name, "KEY_MOUSE") == 0) {
				MEVENT event;

				getmouse(&event);
				cursory = sct_mouseinputy(event.y);
				line = text;
				for(int i = 0; i < cursory; i++) {
					if(line->next) {
						line = line->next;
					}else{
						cursory = i;
					}
				}
				cursorx = sct_mouseinputx(line,event.x);
				if(event.bstate==BUTTON1_DOUBLE_CLICKED) {
					// TODO: Select Word
					
				}else if(event.bstate==BUTTON1_TRIPLE_CLICKED) {
					// TODO: Select Line
					
				}
				sct_draw(w, text, line, &cursorx,&cursory, sln);
			}
			else if(strcmp(name, "KEY_RESIZE") == 0) {
				width = getmaxx(w);
				height = getmaxy(w);
				sct_title(width, height, filename, saved);
				sct_draw(w, text, line, &cursorx,&cursory, sln);
				sct_notify("Resized.");
			}
			else {
				sct_insert(w, text, line, &cursorx, &cursory,
					name[0], sln);
				if(saved) {
					saved = 0;
					sct_title(width,height,filename,saved);
				}
				sct_notify("Edit.");
				// Character Insert
//				printf("NAME: %s", name);
			}
		}
	}
	endwin();
	// Return 0 on success.
	return 0;
}
