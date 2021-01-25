#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ncurses.h>

#include "tarball.h"

#define ENTER 10

#define BORDER_COLOR 1

struct view
{
    WINDOW *win; 
    char **filelist; 
    int size; 
    int start; 
    int i; 
} file_view, queue_view; 

WINDOW *progress_bar; 

char HOME[256]; 

void setup(); 

void load_dir(char *, struct view *); 
void clear_view(struct view *); 

void switch_view(struct view **); 

void show_view(const struct view *); 
void move_cursor(struct view *, int); 

void append_dir(char *, char *); 

void add_file(struct view *, char *); 
void remove_file(struct view *, int); 

void update_progress(double); 

void cleanup(); 

int main()
{
    setup(); 

    char cwd[256]; 
    strcpy(cwd, HOME); 

    struct view *cur_view = &file_view; 
    wattrset(cur_view->win, COLOR_PAIR(BORDER_COLOR)); 
    box(cur_view->win, 0, 0); 

    show_view(&file_view); 
    show_view(&queue_view); 

    int ch; 
    while((ch = wgetch(cur_view->win)) != KEY_F(1))
    {
        if(ch == ' ')
            switch_view(&cur_view); 
        else if(ch == ENTER)
        {
            if(cur_view == &file_view)
            {
                char *fname = file_view.filelist[file_view.i]; 
                add_file(&queue_view, fname);  
                show_view(&queue_view); 
            }
            else if(cur_view == &queue_view)
            {
                struct tarball *tar = new_tarball("a.tar", queue_view.filelist); 
                double p; 
                while((p = archive(tar, 20)) < 1)
                {
                    update_progress(p); 
                    sleep(1); 
                }
                update_progress(1); 
                del_tarball(tar); 
            }
        }
        else if(ch == KEY_BACKSPACE)
        {
            if(cur_view == &queue_view)
                remove_file(&queue_view, queue_view.i);   
        }
        else 
            move_cursor(cur_view, ch); 

        show_view(cur_view); 
    } 

    cleanup(); 
}

void gen_wins()
{
    int view_height = LINES * 0.7; 
    int view_width = (COLS * 0.9) / 2; 

    int spacing = (COLS - view_width * 2) / 3; 

    int view_starty = (LINES - view_height) / 2;  
    int fview_startx = spacing; 
    int qview_startx = fview_startx + view_width + spacing; 

    file_view.win = newwin(view_height, view_width, view_starty, fview_startx); 
    box(file_view.win, 0, 0); 
    wrefresh(file_view.win); 
    keypad(file_view.win, TRUE); 

    queue_view.win = newwin(view_height, view_width, view_starty, qview_startx);  
    box(queue_view.win, 0, 0); 
    wrefresh(queue_view.win); 
    keypad(queue_view.win, TRUE); 

    int progress_height = ((LINES - view_height) / 2) * 0.7; 
    int progress_width = COLS * 0.7; 
    int progress_starty = (((LINES - view_height) / 2) - progress_height) / 2; 
    int progress_startx = (COLS - progress_width) / 2; 

    progress_bar = newwin(progress_height, progress_width, progress_starty, progress_startx);  
    box(progress_bar, 0, 0); 
    wrefresh(progress_bar); 
}

void setup_views() 
{
    load_dir(HOME, &file_view); 

    queue_view.filelist = NULL; 
    queue_view.size = 0; 
    queue_view.i = queue_view.start = 0; 
}

void setup_color()
{
    start_color(); 
    use_default_colors(); 

    init_pair(BORDER_COLOR, COLOR_BLUE, -1); 
}

void setup()
{
    initscr(); 
    refresh(); 
    cbreak(); 
    noecho(); 
    curs_set(0); 

    getcwd(HOME, sizeof(HOME));  
    gen_wins(); 
    setup_views(); 

    setup_color(); 
}

int compar_filename(const void *a, const void *b)
{
    const char *s1 = *(const char **) a; 
    const char *s2 = *(const char **) b; 
    return strcmp(s1, s2); 
}

void load_dir(char *dirname, struct view *view)
{
    int n = 0;  
    clear_view(view); 
    view->filelist = NULL; 

    DIR *dir = opendir(dirname); 
    struct dirent *entry = readdir(dir); 
    while(entry)
    {
        n++; 
        view->filelist = (char **) realloc(view->filelist, sizeof(char *) * n); 
        view->filelist[n-1] = (char *) malloc(sizeof(char) * (strlen(entry->d_name)+1)); 
        strcpy(view->filelist[n-1], entry->d_name); 
        entry = readdir(dir); 
    }
    view->filelist = (char **) realloc(view->filelist, sizeof(char *) * (n+1)); 
    view->filelist[n] = NULL; 

    qsort(view->filelist, n, sizeof(char *), compar_filename); 

    view->size = n; 
    view->i = view->start = 0; 
}

void clear_view(struct view *view)
{
    for(int i = 0; i < view->size; ++i)
        free(view->filelist[i]); 
    free(view->filelist); 
}

void switch_view(struct view **view)
{
    wattrset((*view)->win, A_NORMAL); 
    box((*view)->win, 0, 0); 
    wrefresh((*view)->win); 

    if(*view == &file_view) *view = &queue_view; 
    else                    *view = &file_view; 

    wattrset((*view)->win, COLOR_PAIR(BORDER_COLOR)); 
    box((*view)->win, 0, 0); 
    wrefresh((*view)->win); 
}

void show_view(const struct view *view) 
{
    int height = getmaxy(view->win) - 2; 
    int width = getmaxx(view->win) - 2;

    wattrset(view->win, A_NORMAL); 

    for(int i = 0; i < height; ++i)
        mvwhline(view->win, i+1, 1, ' ', width); 

    for(int i = 0; view->start + i < view->size && i < height; ++i)
        mvwprintw(view->win, i+1, 1,
                view->filelist[view->start + i]);  

    if(view->size > 0)
        mvwchgat(view->win,
                (view->i - view->start) + 1, 1,
                width, A_REVERSE, 0, NULL); 

    wrefresh(view->win); 
}

void move_cursor(struct view *view, int ch)
{
    int width = getmaxx(view->win) - 2; 
    if(view->i > 0 && ch == KEY_UP)
    {
        view->i--; 
        if(view->i < view->start)
            view->start = view->i; 
    }
    else if(view->i < view->size-1 && ch == KEY_DOWN)
    {
        view->i++; 
        if(view->i > view->start + width)
            view->start = view->i - width;  
    }
}

void append_dir(char *cwd, char *dir)
{
    if(!strcmp(dir, ".."))
        *strrchr(cwd, '/') = 0;  
    else if(strcmp(dir, "."))
    {
        strcat(cwd, "/"); 
        strcat(cwd, dir); 
    }
}

void add_file(struct view *view, char *filename)
{
    view->size++; 
    int n = view->size; 

    view->filelist = (char **) realloc(view->filelist, sizeof(char *) * (n+1)); 
    view->filelist[n-1] = (char *) malloc(sizeof(char) * (strlen(filename)+1)); 
    strcpy(view->filelist[n-1], filename); 

    view->filelist[n] = NULL; 
}

void remove_file(struct view *view, int index)
{
    if(index < 0 || index >= view->size)
        return; 

    free(view->filelist[index]); 

    view->size--; 
    int n = view->size; 

    char **tmp = view->filelist; 
    view->filelist = (char **) malloc(sizeof(char *) * (n+1)); 
    for(int i = 0; i < index; ++i)
        view->filelist[i] = tmp[i]; 
    for(int i = index; i < view->size; ++i)
        view->filelist[i] = tmp[i+1]; 
    free(tmp); 

    view->filelist[n] = NULL; 
}

void update_progress(double progress)
{
    int height = getmaxy(progress_bar) - 2; 
    int width = getmaxx(progress_bar) - 2; 
    
    int complete = width * progress; 
    for(int i = 0; i < complete; ++i)
        mvwvline(progress_bar, 1, i+1, 
                ' ' | A_REVERSE, height); 
    wrefresh(progress_bar); 
}

void del_wins()
{
    delwin(file_view.win); 
    delwin(queue_view.win); 
    delwin(progress_bar); 
}

void cleanup_views()
{
    clear_view(&file_view); 
    clear_view(&queue_view); 
}

void cleanup()
{
    del_wins(); 
    cleanup_views(); 
    endwin(); 
}
