#include "mime_types.h"

static int mime_types_add(char *ext, char *mime, cache *c) {
	char *value = strdup(mime);

	cache_add (c, ext, value);
	return 0;
}

/* load all mime types */
cache *mime_types_init() {
	cache *c = calloc(1, sizeof(*c));
	cache_init (c);
	
	mime_types_add (".pdf"    , "application/pdf", c);
	mime_types_add (".sig"    , "application/pgp-signature", c);
	mime_types_add (".spl"    , "application/futuresplash", c);
	mime_types_add (".class"  , "application/octet-stream", c);
	mime_types_add (".ps"     , "application/postscript", c);
	mime_types_add (".torrent", "application/x-bittorrent", c);
	mime_types_add (".dvi"    , "application/x-dvi", c);
	mime_types_add (".gz"     , "application/x-gzip", c);
	mime_types_add (".pac"    , "application/x-ns-proxy-autoconfig", c);
	mime_types_add (".swf"    , "application/x-shockwave-flash", c);
	mime_types_add (".tar.gz" , "application/x-tgz", c);
	mime_types_add (".tgz"    , "application/x-tgz", c);
	mime_types_add (".tar"    , "application/x-tar", c);
	mime_types_add (".zip"    , "application/zip", c);
	mime_types_add (".mp3"    , "audio/mpeg", c);
	mime_types_add (".m3u"    , "audio/x-mpegurl", c);
	mime_types_add (".wma"    , "audio/x-ms-wma", c);
	mime_types_add (".wax"    , "audio/x-ms-wax", c);
	mime_types_add (".ogg"    , "application/ogg", c);
	mime_types_add (".wav"    , "audio/x-wav", c);
	mime_types_add (".gif"    , "image/gif", c);
	mime_types_add (".jar"    , "application/x-java-archive", c);
	mime_types_add (".jpg"    , "image/jpeg", c);
	mime_types_add (".jpeg"   , "image/jpeg", c);
	mime_types_add (".png"    , "image/png", c);
	mime_types_add (".xbm"    , "image/x-xbitmap", c);
	mime_types_add (".xpm"    , "image/x-xpixmap", c);
	mime_types_add (".xwd"    , "image/x-xwindowdump", c);
	mime_types_add (".css"    , "text/css", c);
	mime_types_add (".html"   , "text/html", c);
	mime_types_add (".htm"    , "text/html", c);
	mime_types_add (".js"     , "text/javascript", c);
	mime_types_add (".asc"    , "text/plain", c);
	mime_types_add (".c"      , "text/plain", c);
	mime_types_add (".cpp"    , "text/plain", c);
	mime_types_add (".log"    , "text/plain", c);
	mime_types_add (".conf"   , "text/plain", c);
	mime_types_add (".text"   , "text/plain", c);
	mime_types_add (".txt"    , "text/plain", c);
	mime_types_add (".dtd"    , "text/xml", c);
	mime_types_add (".xml"    , "text/xml", c);
	mime_types_add (".mpeg"   , "video/mpeg", c);
	mime_types_add (".mpg"    , "video/mpeg", c);
	mime_types_add (".mov"    , "video/quicktime", c);
	mime_types_add (".qt"     , "video/quicktime", c);
	mime_types_add (".avi"    , "video/x-msvideo", c);
	mime_types_add (".asf"    , "video/x-ms-asf", c);
	mime_types_add (".asx"    , "video/x-ms-asf", c);
	mime_types_add (".wmv"    , "video/x-ms-wmv", c);
	mime_types_add (".bz2"    , "application/x-bzip", c);
	mime_types_add (".tbz"    , "application/x-bzip-compressed-tar", c);
	mime_types_add (".tar.bz2", "application/x-bzip-compressed-tar", c);
	mime_types_add (".default", "application/octet-stream", c); /* default */
	
	return c;
}

char *mime_types_get(char *ext, int len, cache *c) {
	if (len > 1) {
		if (cache_exists(c, ext, len) == 0) {
			return cache_get(c, ext, len);
		} else {
			return cache_get(c, ".default", 8);
		}
	} else {
		return cache_get(c, ".default", 8);
	}
}

char *mime_types_get_by_filename(char *filename, int len, cache *c) {
	char ext[17];
	int ext_len = 0;

	ext[16] = '\0';

	while (--len) {
		if (ext_len < 16) {
			ext[15-ext_len] = filename[len];
			++ext_len;
		}
		
		if (filename[len] == '.') {
			break;
		}
	}

	char *extension = ext;
	extension = extension + (16-ext_len);

	return mime_types_get(extension, ext_len, c);
}

int mime_types_free(cache *c) {
	cache_free (c);
	return 0;
}

