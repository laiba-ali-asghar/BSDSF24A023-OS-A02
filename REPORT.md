# OS Programming Assignment 02 — Report
**Name:** Laiba Ali
**Roll No:** BSDSF24A023

## Feature 2: ls-v1.1.0 — Long Listing Format

### Q1. What is the difference between stat() and lstat()? When should ls use lstat()?

Both functions get information about a file. The difference is how they handle symbolic links:

- `stat()` follows the link and gives information about the file the link points to.
- `lstat()` does not follow the link and gives information about the link itself.

For normal files and directories, both give the same result.

`ls -l` should use `lstat()`, so a symbolic link is shown as a link (its permissions start with `l`) instead of looking like the file it points to. That is why my `print_long()` function uses `lstat()`.

### Q2. How do bitwise operators and macros extract file type and permissions from st_mode?

`st_mode` is a single integer in which different bits store the file type and the permissions.

**Permissions:** each permission macro (such as `S_IRUSR` for owner read) has only one bit set. Using bitwise AND (`&`) checks whether that bit is on:

    if (mode & S_IRUSR) str[1] = 'r';

If the result is non-zero, the permission is present. The same is done with `S_IWUSR`, `S_IXUSR` (owner), `S_IRGRP`, `S_IWGRP`, `S_IXGRP` (group), and `S_IROTH`, `S_IWOTH`, `S_IXOTH` (others).

**File type:** the type is stored in a group of bits. `S_IFMT` is a mask that selects those bits, and the result is compared with a type value:

    if ((mode & S_IFMT) == S_IFDIR)   // directory

The macro `S_ISDIR(mode)` does the same check more simply, which is what my code uses (similarly `S_ISLNK()` for links).

## Feature 3: ls-v1.2.0 — Column Display (Down Then Across)

### Q1. Explain the logic for "down then across" printing. Why is a single loop insufficient?

First, all filenames are read into an array and the longest name is found. The number of columns is calculated as `terminal_width / (longest_name + 2)`, and the number of rows as `count / columns` rounded up.

Then the output is printed row by row. In each row, the program prints one item from each column using the index `row + col * rows`. For example, row 0 prints items 0, rows, 2*rows, and so on.

A single loop prints items in order (0, 1, 2, 3...) from left to right, which gives an "across" layout. In "down then across", items that are next to each other in the array must appear one below the other, so the program has to jump through the array using the formula. It also needs the total count and number of rows before printing anything, which a single loop does not know.

### Q2. What is the purpose of ioctl here? What are the limitations of a fixed 80-column width?

`ioctl()` with the `TIOCGWINSZ` request asks the terminal for its current size. `ws_col` gives the width in characters, so the program can fit as many columns as the screen allows.

With a fixed width of 80:
- On a wide terminal, space is wasted because fewer columns are printed than could fit.
- On a narrow terminal (less than 80), lines become too long and wrap, which breaks the column alignment.
- The output does not adjust when the user resizes the window.
## Feature 4: ls-v1.3.0 — Horizontal Column Display (-x)

### Q1. Compare the complexity of vertical and horizontal printing. Which needs more pre-calculation?

Horizontal printing is simpler. It goes through the names in order and keeps track of the current position on the line. If the next name does not fit, it prints a newline and continues.

Vertical ("down then across") printing needs more pre-calculation. Before printing anything, it must know the total number of files, the number of columns, and the number of rows, because it uses the formula `row + col * rows` to decide which item goes where. Horizontal printing only needs the column width and terminal width.

### Q2. How does your program manage the different display modes?

I used one integer variable `mode` with three constants: `MODE_DEFAULT`, `MODE_LONG`, and `MODE_HORIZONTAL`. It starts as `MODE_DEFAULT`. In the `getopt()` loop, `-l` sets it to `MODE_LONG` and `-x` sets it to `MODE_HORIZONTAL`.

The mode is passed to `do_ls()`. After reading all filenames into the array, `do_ls()` uses an if/else chain: `MODE_LONG` calls `print_long()` for each file, `MODE_HORIZONTAL` calls `print_horizontal()`, and otherwise it calls `print_columns()`.

## Feature 5: ls-v1.4.0 — Alphabetical Sort

### Q1. Why must all entries be read into memory before sorting? What are the drawbacks for very large directories?

`readdir()` returns entries one at a time in no particular order. To sort, the program must compare every name with the others, so all names must be available at the same time. That is why they are stored in a dynamic array first and sorted with `qsort()` before printing.

For a directory with millions of files:
- It uses a lot of memory, because every name is stored at once.
- Nothing is printed until all entries are read and sorted, so the user waits longer.
- The array has to be resized many times with `realloc()`, which takes extra time.

### Q2. Explain the purpose and signature of the comparison function for qsort(). Why must it take const void * arguments?

`qsort()` can sort any type of data, so it needs a comparison function to tell it how to compare two items. My function is:

    int compare_names(const void *a, const void *b)

It returns a negative number if the first item should come first, 0 if they are equal, and a positive number if the second should come first.

The arguments are `const void *` because `qsort()` is generic and does not know the data type; `void *` can point to anything. Inside the function, I cast them to `const char **`, because each element in my array is a `char *` (a string), so a pointer to an element is a `char **`. Then I dereference to get the strings and compare them with `strcmp()`. `const` means the function does not change the data.
## Feature 6: ls-v1.5.0 — Colorized Output

### Q1. How do ANSI escape codes produce color? Show the code for green text.

ANSI escape codes are special character sequences that the terminal treats as instructions instead of printing them. They start with the escape character `\033`, followed by `[`, one or more numbers, and `m`. The numbers choose the style or color (for example, 31 is red, 32 is green, 34 is blue). The code `\033[0m` resets back to normal.

To print text in green:

    printf("\033[0;32mHello\033[0m\n");

`\033[0;32m` turns on green, `Hello` is printed in green, and `\033[0m` resets the color.

### Q2. Which bits in st_mode show whether a file is executable by owner, group, or others?

There are three execute bits:
- `S_IXUSR`: executable by the owner
- `S_IXGRP`: executable by the group
- `S_IXOTH`: executable by others

Each is checked with bitwise AND. In my code, I combine them with OR and check whether any of them is set:

    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))   // executable → green