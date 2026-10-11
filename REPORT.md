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