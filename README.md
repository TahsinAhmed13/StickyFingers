# StickyFingers

A command line file archiving tool. 

# Team

Members: Tahsin, Taysen, and Tazer\
Pd: 10

# Description 

## Background 

In most modern computers file archiving and compression are necessary. Unix accomplishes this in two steps. First a program called `tar` concatenates these files into one, which another program, either `gzip` or `bzip2`, compress using algorithms that usually look at word frequencies. Our program focuses on the former, because compression is hard and beyond the scope of this class.  
\
The way `tar` works is rather simple. It divides files into 512 byte blocks. Each file in the archive has a preceding header block, as specified by the **ustar** standard, and extra spaces are filled with NULL characters. The last block is also just NULL characters. More information about `tar` can be found [here](https://en.wikipedia.org/wiki/Tar_(computing)).  
\
Our program will implement the `tar` archiving algorithm as well as provide a UI to easily archive and extract files. There will also be a priority queue, so that users can either prioritize bigger or smaller files first.    

## User Interface

![conceptual](scrots/conceptual.png)

1. Progress Bar: progress on current item and information on queued items
2. File View: chose and view the contents of files
3. Queue View: view and edit queue items
4. Controls: quick reference of commands

## Systems Concepts

1. Allocating Memory: items in priority queue will have to dynamically allocated
2. Working with files: files are read from and written to in chuncks
3. File information: file metadata is used to create the tarball and some information will be displayed in the file view
4. Process: a background process will create the tarball, so the user can continue making and editing tars
5. Signals: `Ctrl+C` will be overwritten to ask the user if they wish to discard the current item
6. Shared Memory: the main process will add to the priority queue, while the background process will remove them as it finishes with them

## Timeline

1. Implement taring algorithm (2-3 days)
2. Implement priority queue (1 day)
3. Create UI interface (3 days)
    - Select files from file view (1 day)
    - Edit items in queue view (1 day)
    - Progress bar and signals (1 day)
5. Finish README (1-2 hrs)

## Libraries 

* ncurses: used to create a command line interface. 
  
Mac OSX:
```
brew install ncurses
```

Debian/Ubuntu:
```
sudo apt-get install libncurses-dev
```

Arch/Manjaro:
```
sudo pacman -Syu ncurses
```

---

# DEVLOG

Sun Jan 17, 2021 8:33 PM - Tahsin Ahmed\
Implemented archiving tars.\
\
Fri Jan 22, 2021 12:45 AM - Tahsin Ahmed\
Add tarball struct to store progress of an archive event. Also redefined archive function to be more modular.\
\
Sat Jan 23, 2021 7:15 PM - Tahsin Ahmed\
Implemented extracting tars.\
\
Mon Jan 25, 2021 3:30 AM - Tahsin Ahmed\
Add user interface.\
\
Mon Jan 25, 2021 9:48 AM - Tahsin Ahmed\
Add loading bar. Kind of works...\
\

---

# How to Use

To run: 
```
make && make run
```

Controls: 
* Arrow keys to navigate
* Space to switch between views (view is highlighted)
* Enter on file view (left) to add item
* Backspace on queue view (right) to remove items
* Enter on queue view to process
* F1 to exit 

Program will show progress bar as items are being archived. It will output an a.tar that is compatible with the Unix `tar` utility. 

# Bugs and Shortcomings 

* There isn't actually a queue. Queue view shows the next things to be processed. It is named after what it is supposed to be. :(
* There are no forked processes. The program is mostly idle while tar is processing.
* Tar file is defaulted to `a.tar`. Creating a popup window to get input would take too long.
* The program cannot extract, although extraction is implemented. This can be tested through `tarball_test.c` and `make test`.
