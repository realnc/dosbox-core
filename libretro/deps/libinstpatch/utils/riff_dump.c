/*
 * riff_dump.c - Command line utility to dump info about RIFF files
 *
 * riff_dump utility (licensed separately from libInstPatch)
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 * Public Domain, use as you please.
 *
 * Usage:
 *  riff_dump [OPTION] file.sf2
 *
 * Help options:
 *   -h, --help                 Display help options
 *
 * Application options :
 *   -d, --dump=CHUNK_INDEX     Dump a chunk by CHUNK_INDEX index
 *   -t, --dump-type=CHNK       Dump a chunk by RIFF FOURCC CHNK
 *   -r, --raw                  Do raw dump rather than formatted hex dump
 *
 */
#include <string.h>
#include <libinstpatch/libinstpatch.h>

static gboolean recurse_riff_chunks(IpatchRiff *riff, char *indent,
                                    GError **err);
static void display_chunk(IpatchRiff *riff, char *indent);
static gboolean dump_chunk(IpatchRiff *riff, GError **err);

/* options arguments (global for convenience)*/
static gint dump_index = -1;      /* set to chunk index if chunk dump requested */
static gchar *dump_type = NULL;   /* set to 4 char string if dumping a chunk type */
static gboolean raw_dump = FALSE; /* set to TRUE for raw byte dumps */

/* dislaying variables (global for convenience)*/
static int chunk_index = 0;       /* current index */
static gboolean display = TRUE;   /* set to FALSE to not display chunks */
static gboolean stop = FALSE;     /* set to TRUE to stop recursion */

int
main(int argc, char *argv[])
{
    /* riff file variables */
    char *file_name = NULL; /* file name */
    IpatchFile *riff_file; /* riff file to open */
    IpatchFileHandle *fhandle = NULL; /* riff file file handle */
    IpatchRiff *riff;
    IpatchRiffChunk *chunk;
    char indent_buf[256] = ""; /* indentation buffer */

    GError *err = NULL;
    static gchar **file_arg = NULL; /* file name argument */

    /* parsing context variables */
    GOptionContext *context = NULL; /* parsing context */

    /* option entries to parse */
    static GOptionEntry entries[] =
    {
        { "dump", 'd', 0, G_OPTION_ARG_INT, &dump_index, "Dump a chunk by CHUNK_INDEX index", "CHUNK_INDEX" },
        { "dump-type", 't', 0, G_OPTION_ARG_STRING, &dump_type, "Dump a chunk by RIFF FOURCC CHNK", "CHNK" },
        { "raw", 'r', 0, G_OPTION_ARG_NONE, &raw_dump, "Do raw dump rather than formatted hex dump", NULL },
        { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &file_arg, NULL, "file.sf2" },
        { NULL }
    };

    /* parse option in command line */
    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);

    if(!g_option_context_parse(context, &argc, &argv, &err))
    {
        g_print("option parsing failed: %s\n", err->message);
        g_error_free(err);
        g_strfreev(file_arg);
        g_free(dump_type);
        return (1);
    }

    /* prepare variable for displaying */
    if(dump_index >= 0)
    {
        display = FALSE;  /* we enable display when we find chunk */
    }
    else if(dump_type)
    {
        display = FALSE;
    }

    /* take the file name argument */
    if(file_arg)
    {
        file_name = *file_arg;
    }

    /* libinstpatch initialization */
    ipatch_init();

    riff_file = ipatch_file_new();

    if(file_name)
    {
        fhandle = ipatch_file_open(riff_file, file_name, "r", &err);
    }

    if(!(fhandle))
    {
        fprintf(stderr, "Failed to open file '%s': %s\n",
                file_name, err ? err->message : "<no details>");
        g_free(dump_type);
        g_strfreev(file_arg);
        return (1);
    }

    riff = ipatch_riff_new(fhandle);

    /* start reading chunk */
    if(!(chunk = ipatch_riff_start_read(riff, &err)))
    {
        fprintf(stderr, "Failed to start RIFF parse of file '%s': %s\n",
                file_name, err ? err->message : "<no details>");
        g_free(dump_type);
        g_strfreev(file_arg);
        return (1);
    }

    /* if a dump of chunk 0 requested or type matches, display everything */
    if(dump_index == 0
            || (dump_type && strncmp(dump_type, chunk->idstr, 4) == 0))
    {
        display = TRUE;
    }

    if(display)
    {
        display_chunk(riff, indent_buf);
    }

    chunk_index++;
    strcat(indent_buf, "  ");

    /* continue to read by recursion */
    if(!recurse_riff_chunks(riff, indent_buf, &err))
    {
        fprintf(stderr, "%s\n", ipatch_riff_message_detail
                (riff, -1, "Error while parsing RIFF file '%s': %s",
                 file_name, err ? err->message : "<no details>"));
        g_free(dump_type);
        g_strfreev(file_arg);
        return (1);
    }

    g_free(dump_type); /* free dump type */
    dump_type = NULL;  /* not needed but a good practice */
    g_strfreev(file_arg);  /* free file arguments array */
    file_arg = NULL;   /* not needed but a good practice */

    return (0);
}

gboolean
recurse_riff_chunks(IpatchRiff *riff, char *indent, GError **err)
{
    IpatchRiffChunk *chunk;
    gboolean retval;

    while(!stop && (chunk = ipatch_riff_read_chunk(riff, err)))
    {
        if(dump_index == chunk_index)  /* dump by chunk index match? */
        {
            if(chunk->type != IPATCH_RIFF_CHUNK_SUB)  /* list chunk? */
            {
                display_chunk(riff, indent);

                strcat(indent, "  ");
                display = TRUE;
                retval = recurse_riff_chunks(riff, indent, err);

                stop = TRUE;
                return (retval);
            }
            else
            {
                retval = dump_chunk(riff, err);  /* hex dump of sub chunk */
                stop = TRUE;
                return (retval);
            }
        } /* dump by type match? */
        else if(dump_type && strncmp(dump_type, chunk->idstr, 4) == 0)
        {
            if(chunk->type != IPATCH_RIFF_CHUNK_SUB)  /* list chunk? */
            {
                display = TRUE;
                strcat(indent, "  ");
                recurse_riff_chunks(riff, indent, err);
                indent[strlen(indent) - 2] = '\0';
                display = FALSE;
            }
            else
            {
                dump_chunk(riff, err);    /* hex dump of sub chunk */
            }
        }
        else			/* no dump match, just do stuff */
        {
            if(display)
            {
                display_chunk(riff, indent);
            }

            chunk_index++;		/* advance chunk index */

            if(chunk->type != IPATCH_RIFF_CHUNK_SUB)	/* list chunk? */
            {
                strcat(indent, "  ");

                if(!recurse_riff_chunks(riff, indent, err))
                {
                    return (FALSE);
                }

                indent[strlen(indent) - 2] = '\0';
            }
        }

        if(!ipatch_riff_close_chunk(riff, -1, err))
        {
            return (FALSE);
        }
    }

    return (ipatch_riff_get_error(riff, NULL));
}

void
display_chunk(IpatchRiff *riff, char *indent)
{
    IpatchRiffChunk *chunk;
    int filepos;

    chunk = ipatch_riff_get_chunk(riff, -1);
    filepos = ipatch_riff_get_position(riff);

    if(chunk->type == IPATCH_RIFF_CHUNK_SUB)
        printf("%s(%.4s)[%4d] (ofs = 0x%x, size = %d)\n", indent,
               chunk->idstr, chunk_index,
               filepos - (chunk->position + IPATCH_RIFF_HEADER_SIZE),
               chunk->size);
    else	/* list chunk */
        printf("%s<%.4s>[%4d] (ofs = 0x%x, size = %d)\n", indent,
               chunk->idstr, chunk_index,
               filepos - (chunk->position + IPATCH_RIFF_HEADER_SIZE),
               chunk->size);
}

#define BUFFER_SIZE (16 * 1024)

/* hex dump of a sub chunk */
gboolean
dump_chunk(IpatchRiff *riff, GError **err)
{
    IpatchRiffChunk *chunk;
    guint8 buf[BUFFER_SIZE];
    int filepos, read_size, bytes_left, i;

    chunk = ipatch_riff_get_chunk(riff, -1);
    filepos = ipatch_riff_get_position(riff);

    if(!raw_dump)
    {
        printf("Dump chunk: (%.4s)[%4d] (ofs = 0x%x, size = %d)",
               chunk->idstr, chunk_index,
               filepos - (chunk->position + IPATCH_RIFF_HEADER_SIZE),
               chunk->size);

        i = filepos & ~0xF;   /* round down to nearest 16 byte offset */

        while(i < filepos)  /* advance to start point in 16 byte block */
        {
            if(!(i & 0xF))
            {
                printf("\n%08u  ", i);    /* print file position */
            }
            else if(!(i & 0x3))
            {
                printf(" |  ");    /* print divider */
            }

            printf("   ");		/* skip 1 byte character */
            i++;
        }
    }

    read_size = BUFFER_SIZE;
    bytes_left = chunk->size;

    while(bytes_left)		/* loop until chunk exhausted */
    {
        if(bytes_left < BUFFER_SIZE)
        {
            read_size = bytes_left;
        }

        if(!ipatch_file_read(riff->handle, &buf, read_size, err))
        {
            return (FALSE);
        }

        for(i = 0; i < read_size; i++, filepos++)
        {
            if(!raw_dump)
            {
                if(!(filepos & 0xF))
                {
                    printf("\n%08u  ", filepos);    /* print file position */
                }
                else if(!(filepos & 0x3))
                {
                    printf(" |  ");    /* print divider */
                }
            }

            printf("%02X ", buf[i]);
        }

        bytes_left -= read_size;
    }

    printf("\n");

    return (TRUE);
}
