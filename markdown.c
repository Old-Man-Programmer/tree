/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

// External variables from tree.c
extern FILE *outfile;
extern char *_nl;
extern bool Fflag;      // For appending file type indicators
extern bool noreport;   // To control report generation
extern bool duflag;     // For -du option in report
extern bool wcflag;     // For word count in report and per-file
extern bool noindent;   // For report formatting
extern bool hflag, siflag; // For psize in report
// Additional flags for metadata in markdown_printfile
extern bool pflag, sflag, uflag, gflag, Dflag, inodeflag, devflag, cflag;

// Helper function to escape Markdown special characters
// Takes FILE* to allow flexibility, though currently uses global outfile via markdown_printfile
static void markdown_escape_print(FILE *out_param, const char *s) {
    if (!s) return;
    while (*s) {
        switch (*s) {
            case '*': /* case '_': */ case '`': case '[': case ']': case '\\':
            case '#': case '+': case '-': /* Period '.' is NOT escaped */ case '!':
                fprintf(out_param, "\\%c", *s);
                break;
            default:
                fprintf(out_param, "%c", *s);
                break;
        }
        s++;
    }
}

void markdown_intro(void) {
    // No specific intro for basic Markdown list output
    return;
}

void markdown_outtro(void) {
    // No specific outtro for basic Markdown list output
    return;
}

int markdown_printinfo(char *dirname, struct _info *file, int level) {
    UNUSED(dirname);
    UNUSED(file);

    // Indentation: two spaces per level
    if (!noindent) { // Respect noindent for the indentation part of the line
        for (int i = 0; i < level; i++) {
            fprintf(outfile, "  ");
        }
    }
    // Markdown bullet
    fprintf(outfile, "- ");
    return 0; // Return 0, indicating no special closing action needed by listdir
}

int markdown_printfile(char *dirname_arg, char *filename_arg, struct _info *file, int descend) {
    UNUSED(dirname_arg);
    UNUSED(descend);

    // Check if any metadata flag is active
    // Ensure 'file' is not NULL before accessing its members for wcflag condition
    bool has_meta = pflag || sflag || uflag || gflag || Dflag || inodeflag || devflag ||
                    (wcflag && file); // Changed: show meta block if wcflag is on, regardless of count

    if (has_meta && file) { // Ensure file is not NULL before trying to print metadata
        fprintf(outfile, "[");
        bool first_item = true;

        // Helper macro to append items with a leading space if not the first item
        #define APPEND_META_ITEM(condition, ...) \
            if (condition) { \
                if (!first_item) fprintf(outfile, " "); \
                fprintf(outfile, __VA_ARGS__); \
                first_item = false; \
            }

        if (inodeflag) APPEND_META_ITEM(true, "%lld", (long long)file->linode);
        if (devflag) APPEND_META_ITEM(true, "%d", (int)file->ldev);
        if (pflag) APPEND_META_ITEM(true, "%s", prot(file->mode));
        if (uflag) APPEND_META_ITEM(true, "%s", uidtoname(file->uid));
        if (gflag) APPEND_META_ITEM(true, "%s", gidtoname(file->gid));
        if (sflag) {
            char size_buf[40];
            psize(size_buf, file->size);
            // psize might prepend a space, remove it for cleaner look inside brackets
            APPEND_META_ITEM(true, "%s", size_buf + (size_buf[0] == ' ' ? 1 : 0));
        }
        // For word count, ensure 'file' is valid. word_count is 0 for non-text files.
        if (wcflag) { // Changed: always print word count if wcflag is on
             APPEND_META_ITEM(true, "%lld wc", (long long)file->word_count);
        }
        if (Dflag) APPEND_META_ITEM(true, "%s", do_date(cflag ? file->ctime : file->mtime));

        #undef APPEND_META_ITEM
        fprintf(outfile, "] ");
    }

    // Determine the name to print: use file->name if available and not empty, otherwise filename_arg (for root dir)
    const char *name_to_print = (file && file->name && file->name[0]) ? file->name : filename_arg;
    markdown_escape_print(outfile, name_to_print);

    if (Fflag && file) { // Append file type character if Fflag is set
        char ftype_char = Ftype(file->mode);
        if (ftype_char) {
            fputc(ftype_char, outfile);
        }
    }

    if (file && file->lnk) { // If it's a symbolic link, ensure 'file' is not NULL
        fprintf(outfile, " -> ");
        markdown_escape_print(outfile, file->lnk);
        if (file->orphan) {
            fprintf(outfile, " [orphan]");
        }
    }
    return 0; // No special closing action needed by listdir
}

int markdown_error(char *error_msg) {
    // Append error to the current line.
    fprintf(outfile, " [Error: ");
    markdown_escape_print(outfile, error_msg); // Use the modified escape print
    fprintf(outfile, "]");
    return 0;
}

void markdown_newline(struct _info *file, int level, int postdir, int needcomma) {
    UNUSED(file);
    UNUSED(level);
    UNUSED(postdir);
    UNUSED(needcomma);
    fprintf(outfile, "%s", _nl); // _nl should be "\n" for Markdown
}

// markdown_close will use null_close from list.c, so no specific function here.

void markdown_report(struct totals tot) {
    char buf[256]; // For psize

    if (noreport) {
        return;
    }

    if (!noindent) {
        fprintf(outfile, "\n---\n\n"); // Markdown horizontal rule and spacing
    } else {
        fprintf(outfile, "\n"); // Still need a newline if noindent is set
    }

    fprintf(outfile, "**Summary:**\n\n");

    fprintf(outfile, "- Directories: %zu\n", tot.dirs);
    fprintf(outfile, "- Files: %zu\n", tot.files);

    if (duflag) { // duflag implies total size of dirs considered
        psize(buf, tot.size);
        // psize might prepend a space, adjust if necessary for cleaner Markdown.
        fprintf(outfile, "- Total size: %s%s\n", buf + (buf[0] == ' ' ? 1 : 0), (hflag || siflag) ? "" : " bytes");
    }

    if (wcflag && (tot.dirs > 0 || tot.files > 0)) { // Only print if wcflag is on and there's something to report
        fprintf(outfile, "- Total word count: %lld\n", (long long int)tot.total_word_count);
    }
}
