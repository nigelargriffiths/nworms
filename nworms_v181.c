#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <curses.h>
#include <sys/utsname.h>
#include <sys/dr.h>

#define VERSION "18.1"
#define LENGTH 10
#define WORM_MAX 50
#define SPIN 1024

int debug = 0;
int spin = 1024;
int worm_count;

void a(),b();
int c();
void a() { int i; for(i=0;i<spin;i++) b(); }
void b() { int i; for(i=0;i<1024;i++) c(); }
int c() { return 42; }


struct coord {
	int c;
	int r;
	struct coord *next;
	};

struct worm {
	char symbol;
	struct coord *start;
	};

struct worm worms[WORM_MAX +1];

void display_worm(struct worm *p)
{
	struct coord *q;
	for(q=p->start;q;q=q->next) 
		mvaddch(q->r,q->c,p->symbol);
}

void init_worm(struct worm *wp, int row, int col, char ch)
{
	int i;
	struct coord **p;

	wp->symbol = ch;
	p = &wp->start;
	for(i=0;i<LENGTH;i++) {
		*p = malloc(sizeof(struct coord));
		if(col < COLS -2)
			(*p)->c = col;
		else
			(*p)->c = COLS/2;
		if(row < LINES -2)
			(*p)->r = row;
		else
			(*p)->r = LINES/2;
		(*p)->next = NULL;
		p = &((*p)->next);
	}
}

int timer1,timer2;

void timer(int interrupt)
{
	signal(SIGALRM,timer);
	alarm(1);
	timer2 = timer1;
	timer1=0;
}
	
void die(int interrupt)
{
	clear();
	refresh();
	endwin();
	exit(0);
}

int found(int row, int col)
{
	int i;
	struct coord *p;

	for(i=0;i<worm_count;i++) 
		for(p=worms[i].start; p!=NULL; p=p->next)
			if( p->r == row && p->c == col)
				return 1;
	return 0;
}

void move_worm(struct worm *p)
{
	struct coord *moved;
	struct coord *end;
	struct coord *old;
	int retry = 0;

retry_again:
	/* save tail */
	moved = p->start;

	/* remove it */
	p->start = p->start->next;
	if(! found(moved->r,moved->c) )
		mvaddch(moved->r,moved->c,' ');

	/* find the end */
	for( end=p->start; end->next != NULL; old=end,end=end->next)
		;
	/* add it back on as the head */
	moved->next=NULL;
	end->next = moved;

	/* move the new head on one space */
	switch( rand() % 16 )
	{
	case 0:
		moved->r=end->r -1;
		moved->c=end->c -1;
		break;
	case 1:
		moved->r=end->r -1;
		moved->c=end->c;
		break;
	case 2:
		moved->r=end->r -1;
		moved->c=end->c +1;
		break;
	case 3:
		moved->r=end->r;
		moved->c=end->c +1;
		break;
	case 4:
		moved->r=end->r +1;
		moved->c=end->c +1;
		break;
	case 5:
		moved->r=end->r +1;
		moved->c=end->c;
		break;
	case 6:
		moved->r=end->r +1;
		moved->c=end->c -1;
		break;
	case 7:
		moved->r=end->r;
		moved->c=end->c -1;
		break;
	default:
		moved->r=end->r*2 - old->r;
		moved->c=end->c*2 - old->c;
		break;
	}
	if( moved->r == old->r && moved->c == old->c && retry++<10 )
		goto retry_again;
	if(moved->r <= 0) moved->r++;
	if(moved->c <= 0) moved->c++;
	if(moved->r >= LINES -1) moved->r--;
	if(moved->c >= COLS -1 ) moved->c--;
	mvaddch(moved->r,moved->c,p->symbol);
}

char serial[64];

void interrupt(int signum)
{
	signal(SIGUSR2, interrupt);
	serial[0]=0;
}

int main(int argc, char **argv)
{
	int i;
	int j;
	int colour = 1;
	int blue = 1;
	struct utsname uts;
        lpar_info_format1_t f1;
	char lparname[64];
	int lparnum;

	if(argc >= 2 && !strcmp(argv[1],"-?") ) {
		printf("nworms version %s Summary\n",VERSION);
		printf("\tWatch those worms wriggle :-)\n");
		printf("\tDisplays the worm speed, Serial number, LPAR number & name\n");
		printf("\tThe worms change colour, if the serial-number or LPAR-name change\n");
		printf("\n");
		printf("nworms: [-?] [[worms] speed]\n");
		printf("\t-?\tThis help information\n");
		printf("\tworms\tThe number of worms between 1 & %d (default 2)\n",WORM_MAX);
		printf("\tspeed\tWorm movement speed between 1 & millions (default 1024)\n");
		printf("\t\t- use this to slow the worms to a realistic speed\n");
		printf("\t\t  so you can see them wriggling\n");
		printf("Examples:\n");
		printf("\tnworms          (defaults to 2 worms & normal speed)\n");
		printf("\tnworms 15       (15 worms & normal speed)\n");
		printf("\tnworms 20 10000 (20 worms & ten times slower)\n");
		printf("\n");
		printf("Stop with Control C\n");
		printf("\n");
		printf("Developer Nigel Griffiths\n");
		printf("Website www.ibm.com/developerworks/wikis/display/WikiPtype/nworms\n");
		exit(0);
	}
		
	if(argc >= 2)
		worm_count = atoi(argv[1]);
	else
		worm_count = 2;
	if(worm_count>WORM_MAX)
		worm_count= WORM_MAX;

	if(argc >= 3)
		spin = atoi(argv[2]);
	else
		spin = SPIN;

	signal(SIGUSR2,interrupt);
	signal(SIGINT,die);
	timer(0);
	srand(getpid());
	initscr();
	noecho();
	leaveok( stdscr, TRUE);
	scrollok( stdscr, FALSE);

colour = has_colors();
start_color();
init_pair((short)0, (short)7, (short)0); /* White */
init_pair((short)1, (short)7, (short)0); /* better Red */
/*init_pair((short)1, (short)1, (short)0);*/ /* Red */
init_pair((short)2, (short)2, (short)0); /* Green */
init_pair((short)3, (short)3, (short)0); /* Yellow */
init_pair((short)4, (short)4, (short)0); /* Blue */
init_pair((short)5, (short)5, (short)0); /* Magenta */
init_pair((short)6, (short)6, (short)0); /* Cyan */
init_pair((short)7, (short)7, (short)0); /* White */

/*
	uname(&uts);
	uts.machine[strlen(uts.machine)-4] = 0;
	strcpy(serial,&uts.machine[2]);
*/
	if(colour) attrset(COLOR_PAIR(4));
	box(stdscr, 0, 0);
	if(colour) attrset(COLOR_PAIR(2));

	for(i=0;i<worm_count;i++) 
		init_worm(&worms[i], 10+i, 10+i, '@'+i);

	for(i=0;;i++) {
		a();

		if(lpar_get_info(LPAR_INFO_FORMAT1, &f1, sizeof(f1)) == 0) {
		    if(f1.lpar_number != -1) {

			if(strcmp(f1.lpar_name,lparname) || f1.lpar_number != lparnum) {
				strcpy(lparname,f1.lpar_name);
				lparnum = f1.lpar_number;

				if(colour) attrset(COLOR_PAIR(5));
			}
		    }
		}	
		else f1.lpar_number = -1;
		uname(&uts);
                uts.machine[strlen(uts.machine)-4] = 0;

                if(strcmp(&uts.machine[2],serial)) {
			strcpy(serial,&uts.machine[2]);
			if(blue) {
				if(colour) attrset(COLOR_PAIR(1));
				blue=0;
			} else {
				if(colour) attrset(COLOR_PAIR(2));
				blue=1;
			}
		}

		for(j=0;j<worm_count;j++) 
			move_worm(&worms[j]);
		timer1++;
		mvprintw(1,1,"Worm moves/s =%d ",timer2);
		mvprintw(2,1,"Serial-Number=%s ",serial);
		if(f1.lpar_number != -1) {
		mvprintw(3,1,"LPAR-Number  =%d ",f1.lpar_number);
		mvprintw(4,1,"LPAR-Name    =%s ",f1.lpar_name);
		}

		refresh();
	}
	clear();
	refresh();
	endwin();
	return 1;
}
