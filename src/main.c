/**
 * SCT Sciences Creamy Text Editor (c) Jeron Lau RAHS Computer Science Club.
 * The Unlicense
**/

#include <stdio.h> // For using printf()
#include <string.h> // For using strlen()
#include <stdint.h> // For using int8_t, uint8_t etc variable types.
#include <ncurses.h> // For getting input, etc.
#include <stdlib.h> // For exit()
#include <unistd.h> // For sleep()

typedef struct {
	int8_t tabs;
	void* prev;
	char text[81];
	void* next;
}textlist_t;

static textlist_t* init_textlist(void) {
	textlist_t* rtn = malloc(sizeof(textlist_t));
	rtn->prev = NULL;
	memset(rtn->text, '\0', 81);
	rtn->next = NULL;
	rtn->tabs = 0;
	return rtn;
}

static void textlist_insert(textlist_t* afterwhat) {
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

static textlist_t* textlist_backspace(textlist_t* deletewhat) {
	textlist_t* prev_item = deletewhat->prev;
	textlist_t* next_item = deletewhat->next;

	if(next_item) next_item->prev = prev_item;
	prev_item->next = next_item;

	free(deletewhat);

	return prev_item;
}

static textlist_t* textlist_delete(textlist_t* deletewhat) {
	textlist_t* next_item = deletewhat->next;

	next_item->prev = NULL;

	free(deletewhat);

	return next_item;
}

static textlist_t* sct_nextline(textlist_t* text, int8_t* y) {
	*y = *y + 1;
	return text->next;
}

static textlist_t* sct_prevline(textlist_t* text, int8_t* y) {
	*y = *y - 1;
	return text->prev;
}

static void sct_exit(uint8_t* running, const char* message) {
	*running = 0;
	endwin();
}

static void sct_draw(WINDOW* w, textlist_t* text, textlist_t* line,
	int8_t* x, int8_t* y)
{
	int i;
	uint8_t infile = 1;

	for(i = 0; i < 10; i++) {
		if(infile) {
			int j;
			for(j = 0; j < text->tabs; j++) {
				mvprintw(i + 1, j * 8, "        ");
			}
			mvprintw(i + 1, text->tabs * 8, "%s\n", text->text);
			if(text->next) {
				text = text->next;
			}else{
				infile = 0;
			}
		}else{
			mvprintw(i + 1, 0, " ~ ~ ~ \n");
		}
	}
	wmove(w, 1 + *y, *x + (line->tabs * 8));
	chgat(1, A_REVERSE, 0, NULL);
	refresh();
}

void sct_insert(WINDOW* w, textlist_t* text, textlist_t* line,
	int8_t* x, int8_t* y, char c)
{
	uint8_t max = 79 - (line->tabs * 8);
	if(*x > max) *x = max;
	if(strlen(line->text) >= max) return;
	memmove(line->text + *x + 1, line->text + *x, strlen(line->text + *x) + 1);
	line->text[*x] = c;
	*x = *x + 1;
	sct_draw(w, text, line, x, y);
}

// Main function.
int main(int argc, char *argv[]) {
	// Variable to store character name in.
	textlist_t* text = init_textlist();
	textlist_t* line = text;
	uint8_t running = 1;
	WINDOW *w = initscr();
	int8_t cursorx = 0, cursory = 0;
	uint8_t mouseHeldDown = 0;

	cbreak();
	noecho();
	nodelay(w, TRUE);
	keypad(w, TRUE);
	meta(w, TRUE);
	nodelay(w, TRUE);
	curs_set(FALSE);
	raw();
	mousemask(ALL_MOUSE_EVENTS, NULL);

	// Print some text
	mvprintw(0, 0, "Science's Creamy Text Editor - SCT");
	sct_draw(w, text, line, &cursorx, &cursory);
	while(running) {
		int32_t chr = getch();
		if(chr != ERR) {
			const char* name = keyname(chr);

			// Close
			if(strcmp(name, "^Q") == 0) {
				sct_exit(&running, "EXITING ON QUIT");
			}
			else if(strcmp(name, "^W") == 0) {
				sct_exit(&running, "EXITING ON FILE CLOSE");
			}
			else if(strcmp(name, "^C") == 0) {
				sct_exit(&running, "EXITING ON FORCE CLOSE");
			}
			// Save
			else if(strcmp(name, "^S") == 0) {
				sct_exit(&running, "EXITING ON SAVE");
			}
			// Functions
			else if(strcmp(name, "KEY_BACKSPACE") == 0) {
				cursorx--;
				if(cursorx < 0) {
					if(line->tabs > 0) {
						line->tabs--;
						cursorx = 0;
					}else if(line->prev != NULL) {
						line = textlist_backspace(line);
						cursory--;
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
				sct_draw(w, text, line, &cursorx, &cursory);
			}
			else if(strcmp(name, "KEY_DC") == 0) {
				sct_exit(&running, "Delete");
				line = textlist_backspace(line->next);
			}
			else if(strcmp(name, "^D") == 0) {
				if(line->prev != NULL) {
					// Delete non-first line
					line = textlist_backspace(line);
					cursory--;
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
				sct_draw(w, text, line, &cursorx, &cursory);
			}
			else if(strcmp(name, "^I") == 0) {
				line->tabs++;
				if(line->tabs > 8) line->tabs = 8;
				sct_draw(w, text, line, &cursorx, &cursory);
			}
			else if(strcmp(name, "KEY_BTAB") == 0) {
				line->tabs--;
				if(line->tabs < 0) line->tabs = 0;
				sct_draw(w, text, line, &cursorx, &cursory);
			}
			else if(strcmp(name, "^J") == 0) {
				textlist_insert(line);
				line = sct_nextline(line, &cursory);
				cursorx = 0;
				sct_draw(w, text, line, &cursorx, &cursory);
				// Newline
			}
			else if(strcmp(name, "^[") == 0) {
				// Alt + other
			}
			else if(strcmp(name, "KEY_UP") == 0) {
				if(line->prev) {
					line = sct_prevline(line, &cursory);
					cursorx = strlen(line->text);
					sct_draw(w, text, line, &cursorx, &cursory);
				}
			}
			else if(strcmp(name, "KEY_DOWN") == 0) {
				if(line->next) {
					line = sct_nextline(line, &cursory);
					cursorx = strlen(line->text);
					sct_draw(w, text, line, &cursorx, &cursory);
				}
			}
			else if(strcmp(name, "KEY_RIGHT") == 0) {
				if(cursorx != strlen(line->text)) cursorx++;
				sct_draw(w, text, line, &cursorx, &cursory);
			}
			else if(strcmp(name, "KEY_LEFT") == 0) {
				if(cursorx != 0) cursorx--;
				sct_draw(w, text, line, &cursorx, &cursory);
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
				if(event.bstate == BUTTON1_PRESSED ||
					event.bstate == BUTTON1_CLICKED ||
					event.bstate == BUTTON1_DOUBLE_CLICKED||
					event.bstate == BUTTON1_TRIPLE_CLICKED )
				{
					cursorx = event.x;
					sct_draw(w, text, line, &cursorx, &cursory);
					mouseHeldDown = 1;
				}
				if(event.bstate == BUTTON1_RELEASED) {
					cursorx = event.x;
					sct_draw(w, text, line, &cursorx, &cursory);
					mouseHeldDown = 0;
//					cursory = event.y - 1;
				}
				if(event.bstate == REPORT_MOUSE_POSITION) {
					cursorx = event.x;
					sct_draw(w, text, line, &cursorx, &cursory);
				}
			}
			else {
				sct_insert(w, text, line, &cursorx, &cursory,
					name[0]);
				// Character Insert
//				printf("NAME: %s\n", name);
			}
		}
		if(mouseHeldDown) {
//			MEVENT event;
//
//			getmouse(&event);
//			cursorx = event.x;
//			sct_draw(w, text, line, &cursorx, &cursory);
		}
	}
	endwin();
	// Return 0 on success.
	return 0;
}
