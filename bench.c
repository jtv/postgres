/*
 * Minimal benchmark for PQgetCopyData alternative.
 *
 * Define CALL to 0 (to use the classic PQgetCopyData) or 1 (to use the
 * proposed new function), then run the binary through "time" to get time and
 * CPU usage stats.
 *
 * DO NOT UPSTREAM THIS FILE.  It's just a demonstration for the prototype
 * patch.
 */
#include <stdio.h>
#include <stdlib.h>

#include <libpq-fe.h>

/* Define CALL to...
 * 0: Use classic PQgetCopyData()
 * 1: Use experimental PQhandleCopyData()
 */

/* Benchmark results (best result per category, out of 4 runs):
 *
 * PQgetCopyData:
 * real - 0m32.972s
 * user - 0m11.364s
 * sys - 0m1.255s
 *
 * PQhandleCopyData:
 * real - 0m32.839s
 * user - 0m3.407s
 * sys - 0m0.872s
 */

#if CALL == 1
/*
 * Print line, add newline.
 */
static int
print_row_and_newline(void *, const char *buf, size_t len)
{
	fwrite(buf, 1, len, stdout);
	return 0;
}
#endif


int
main()
{
#if !defined(CALL)
#error "Set CALL: 0 = PQgetCopyDta, 1 = PQhandleCopyData."
#elif CALL == 0
	fprintf(stderr, "Testing classic PQgetCopyData().\n");
#elif CALL == 1
	fprintf(stderr, "Testing experimental PQhandleCopyData.\n");
#else
#error "Unknown CALL value."
#endif

	PGconn	   *cx = PQconnectdb("");

	if (!cx)
	{
		fprintf(stderr, "Could not connect.\n");
		exit(1);
	}
	PGresult   *tx = PQexec(cx, "BEGIN");

	if (!tx)
	{
		fprintf(stderr, "No result from BEGIN!\n");
		exit(1);
	}
	int			s = PQresultStatus(tx);

	if (s != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to start transaction: status %d.\n", s);
		exit(1);
	}

	PGresult   *r = PQexec(
						   cx,
						   "COPY ("
						   "SELECT generate_series, 'row #' || generate_series "
						   "FROM generate_series(1, 100000000)"
						   ") TO STDOUT"
	);

	if (!r)
	{
		fprintf(stderr, "No result!\n");
		exit(1);
	}
	int			status = PQresultStatus(r);

	if (status != PGRES_COPY_OUT)
	{
		fprintf(stderr, "Failed to start COPY: status %d.\n", status);
		exit(1);
	}

	int			bytes;
#if CALL == 0
	char	   *buffer = NULL;

	for (
		 bytes = PQgetCopyData(cx, &buffer, 0);
		 bytes > 0;
		 bytes = PQgetCopyData(cx, &buffer, 0)
		)
	{
		if (buffer)
		{
			printf("%s", buffer);
			PQfreemem(buffer);
		}
	}
#elif CALL == 1
	while ((bytes = PQhandleCopyData(cx, print_row_and_newline, NULL, 0)) > 0);
#else
#error "Unknown CALL value."
#endif

	if (bytes != -1)
	{
		fprintf(stderr, "Got unexpected result: %d.\n", bytes);
		exit(1);
	}

	/* (Don't bother cleaning up.) */
}
