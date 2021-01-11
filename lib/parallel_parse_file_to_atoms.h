#ifndef PARALLEL_PARSE_FILE_TO_ATOMS_H
#define PARALLEL_PARSE_FILE_TO_ATOMS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <omp.h>

#include "debug.h"
#include "macros/cpp_defines.h"


struct file_atoms {
	char * string;       // The whole file.
	long len;            // 'string' length (bytes).
	char ** atoms;       // Array with the addresses of the atoms in 'string'.
	long * atom_len;     // Length of each atom.
	long num_atoms;      // Number of atoms.
};


void
file_atoms_destructor(struct file_atoms ** obj_ref)
{
	struct file_atoms * obj = *obj_ref;
	free(obj->atom_len);
	free(obj->atoms);
	free(obj->string);
	free(obj);
}


static inline
long
string_delimiter_word(char *str, long s, long e)
{
	long num = 0;
	long i;
	for (i=s;i<e;i++)
		switch (str[i]) {
		case '\r':
		case '\t':
		case '\n':
		case ' ':
		case '\0':
			str[i] = 0;
			num++;
		}
	return num;
}


static inline
long
string_delimiter_line(char *str, long s, long e)
{
	long num = 0;
	long i;
	for (i=s;i<e;i++)
		if (str[i] == '\n' || str[i] == '\0')
		{
			str[i] = 0;
			num++;
		}
	return num;
}


static inline
long
string_delimiter_eof(char *str, long s __attribute__((unused)), long e)
{
	long num = 0;
	if (str[e-1] == '\0')
	{
		str[e-1] = 0;
		num++;
	}
	return num;
}


/*
 * 'str' is a NULL terminated string.
 * When the source is a file, it is the file data followed by a NULL character.
 *
 * keep_empty: keep atoms of zero length
 */
static inline
struct file_atoms * 
string_to_atoms(char *str, long n, long string_delimiter(char *, long, long), int keep_empty)
{
	struct file_atoms * A;
	int num_threads = omp_get_max_threads();
	long block_size = (n + num_threads - 1) / num_threads;
	long total_atoms = 0;
	long i;

	// Thread private data, written only once.
	struct {
		long b_s;             // String block start (inclusive).
		long b_e;             // String block end (exclusive).
		long num_atoms;       // Number of private atoms.
		long offset;          // Offset of private atoms in the total atoms array.
	} tpd[num_threads];

	// Delimit atoms, replacing delimiter-chars with NULL.
	_Pragma("omp parallel")
	{
		int tnum = omp_get_thread_num();
		typeof(&tpd[0]) d = &tpd[tnum];
		d->b_s = tnum * block_size;
		d->b_e = d->b_s + block_size;
		if (d->b_e > n)
			d->b_e = n;
		d->num_atoms = string_delimiter(str, d->b_s, d->b_e);
	}

	// If not keeping empty atoms, count them again.
	if (!keep_empty)
	{
		_Pragma("omp parallel")
		{
			int tnum = omp_get_thread_num();
			typeof(&tpd[0]) d = &tpd[tnum];
			long i, j, num_atoms, len;           // 'j' is always at the end of the previous atom.
			num_atoms = 0;
			for (j=d->b_s-1;((j >= 0) && (str[j] != 0));j--)
				;
			for (i=d->b_s;i<d->b_e;i++)
			{
				if (str[i] != 0)
					continue;
				len = i - j - 1;
				if (len != 0)
					num_atoms++;
				j = i;
			}
			d->num_atoms = num_atoms;
		}
	}

	// Total atoms calculation and memory allocation.
	total_atoms = 0;
	for (i=0;i<num_threads;i++)
	{
		tpd[i].offset = total_atoms;
		total_atoms += tpd[i].num_atoms;
	}
	alloc(1, A);
	A->string = str;
	A->len = n;
	alloc(total_atoms, A->atoms);
	alloc(total_atoms, A->atom_len);
	A->num_atoms = total_atoms;

	// Calculate the addresses and lengths of the atoms.
	_Pragma("omp parallel")
	{
		int tnum = omp_get_thread_num();
		typeof(&tpd[0]) d = &tpd[tnum];
		long i, j, atom_pos, len;
		atom_pos = d->offset;
		for (j=d->b_s-1;((j >= 0) && (str[j] != 0));j--)
			;
		for (i=d->b_s;i<d->b_e;i++)
		{
			if (str[i] != 0)
				continue;
			len = i - j - 1;
			if (keep_empty || (len != 0))
			{
				A->atoms[atom_pos] = &(str[j+1]);
				A->atom_len[atom_pos] = len;
				atom_pos++;
			}
			j = i;
		}
	}

	return A;
}


static
struct file_atoms * 
parse_file_to_atoms(const char * filename, long string_delimiter(char *, long, long), int keep_empty)
{
	struct stat sb;
	char * mem;
	char * str;
	long n;

	int fd = open(filename, O_RDONLY);
	if (fd == -1)
		error("open");
	if (fstat(fd, &sb) == -1)
		error("fstat");
	if (!S_ISREG(sb.st_mode))
		error("not a file\n");
	mem = static_cast(char *,  mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
	if (mem == MAP_FAILED)
		error("mmap");
	if (close(fd) == -1)
		error("close");

	n = sb.st_size+1;
	alloc(n, str);
	str[n-1] = 0;
	_Pragma("omp parallel")
	{
		long i;
		_Pragma("omp for schedule(static)")
		for (i=0;i<sb.st_size;i++)
		{
			str[i] = mem[i];
		}
	}

	if (munmap(mem, sb.st_size) == -1)
		error("munmap");

	// printf("A->string[A->len-1] = '%c'\n", str[n-1]);
	return string_to_atoms(str, n, string_delimiter, keep_empty);
}


// File to space-separated words.
static __attribute__((unused))
struct file_atoms * 
get_words_from_file(const char * filename, int keep_empty)
{
	return parse_file_to_atoms(filename, string_delimiter_word, keep_empty);
}


// File to lines.
static __attribute__((unused))
struct file_atoms * 
get_lines_from_file(const char * filename, int keep_empty)
{
	return parse_file_to_atoms(filename, string_delimiter_line, keep_empty);
}


// Whole file as one string.
static __attribute__((unused))
struct file_atoms * 
get_text_from_file(const char * filename)
{
	return parse_file_to_atoms(filename, string_delimiter_eof, 0);
}



#endif /* PARALLEL_PARSE_FILE_TO_ATOMS_H */



