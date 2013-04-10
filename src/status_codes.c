#include "status_codes.h"

/* load all http status codes */
status_codes *status_codes_init() {
	status_codes *sc = calloc(1, sizeof(*sc));

	sc->HTTP_VER_LEN = 9;
	
	sc->HTTP_VER = malloc(sc->HTTP_VER_LEN+1);
	memcpy (sc->HTTP_VER, "HTTP/1.1 ", sc->HTTP_VER_LEN+1);
	
	sc->HTTP_200_LEN = 6;
	sc->HTTP_302_LEN = 16;
	sc->HTTP_400_LEN = 15;
	sc->HTTP_404_LEN = 13;
	sc->HTTP_500_LEN = 18;
	sc->HTTP_501_LEN = 19;

	sc->HTTP_200 = malloc(sc->HTTP_200_LEN+1);
	memcpy (sc->HTTP_200, "200 OK", sc->HTTP_200_LEN+1);
	
	sc->HTTP_302 = malloc(sc->HTTP_302_LEN+1);
	memcpy (sc->HTTP_302, "302 Object Found", sc->HTTP_302_LEN+1);

	sc->HTTP_400 = malloc(sc->HTTP_400_LEN+1);
	memcpy (sc->HTTP_400, "400 Bad Request", sc->HTTP_400_LEN+1);

	sc->HTTP_404 = malloc(sc->HTTP_404_LEN+1);
	memcpy (sc->HTTP_404, "404 Not Found", sc->HTTP_404_LEN+1);

	sc->HTTP_500 = malloc(sc->HTTP_500_LEN+1);
	memcpy (sc->HTTP_500, "500 Internal Error", sc->HTTP_500_LEN+1);
	
	sc->HTTP_501 = malloc(sc->HTTP_501_LEN+1);
	memcpy (sc->HTTP_501, "501 Not Implemented", sc->HTTP_501_LEN+1);

	/* contents */
	char *content;
	
	content = "<h3>400 Bad Request!</h3>";
	sc->HTTP_400_CONTENT_LEN = strlen(content);
	sc->HTTP_400_CONTENT = malloc(sc->HTTP_400_CONTENT_LEN+1);
	memcpy (sc->HTTP_400_CONTENT, content, sc->HTTP_400_CONTENT_LEN+1);

	content = "<h3>404 Not Found!</h3>";
	sc->HTTP_404_CONTENT_LEN = strlen(content);
	sc->HTTP_404_CONTENT = malloc(sc->HTTP_404_CONTENT_LEN+1);
	memcpy (sc->HTTP_404_CONTENT, content, sc->HTTP_404_CONTENT_LEN+1);

	content = "<h3>500 Internal Error!</h3>";
	sc->HTTP_500_CONTENT_LEN = strlen(content);
	sc->HTTP_500_CONTENT = malloc(sc->HTTP_500_CONTENT_LEN+1);
	memcpy (sc->HTTP_500_CONTENT, content, sc->HTTP_500_CONTENT_LEN+1);

	content = "<h3>501 Not Implemented!</h3>";
	sc->HTTP_501_CONTENT_LEN = strlen(content);
	sc->HTTP_501_CONTENT = malloc(sc->HTTP_501_CONTENT_LEN+1);
	memcpy (sc->HTTP_501_CONTENT, content, sc->HTTP_501_CONTENT_LEN+1);

	return sc;
}

int status_codes_free(status_codes *sc) {
	/* free all codes */
	
	free (sc->HTTP_VER);
	free (sc->HTTP_200);
	free (sc->HTTP_302);
	free (sc->HTTP_400);
	free (sc->HTTP_404);
	free (sc->HTTP_500);
	
	free (sc->HTTP_400_CONTENT);
	free (sc->HTTP_404_CONTENT);
	free (sc->HTTP_500_CONTENT);
	
	free (sc);
	
	return 0;
}

