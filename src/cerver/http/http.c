#include <stdlib.h>

#include "cerver/types/types.h"

#include "cerver/handler.h"
#include "cerver/packets.h"

#include "cerver/http/http.h"
#include "cerver/http/http_parser.h"
#include "cerver/http/route.h"
#include "cerver/http/request.h"

#include "cerver/utils/utils.h"

static void http_receive_handle_default_route (CerverReceive *cr, HttpRequest *request);

#pragma region http

HttpCerver *http_cerver_new (void) {

	HttpCerver *http_cerver = (HttpCerver *) malloc (sizeof (HttpCerver));
	if (http_cerver) {
		http_cerver->cerver = NULL;

		http_cerver->routes = NULL;

		http_cerver->default_handler = NULL;
	}

	return http_cerver;

}

void http_cerver_delete (void *http_cerver_ptr) {

	if (http_cerver_ptr) {
		HttpCerver *http_cerver = (HttpCerver *) http_cerver_ptr;

		dlist_delete (http_cerver->routes);

		free (http_cerver_ptr);
	}

}

HttpCerver *http_cerver_create (Cerver *cerver) {

	HttpCerver *http_cerver = http_cerver_new ();
	if (http_cerver) {
		http_cerver->cerver = cerver;

		http_cerver->routes = dlist_init (http_route_delete, NULL);

		http_cerver->default_handler = http_receive_handle_default_route;
	}

	return http_cerver;

}

void http_cerver_init (HttpCerver *http_cerver) {

	if (http_cerver) {
		// init top level routes
		for (ListElement *le = dlist_start (http_cerver->routes); le; le = le->next) {
			http_route_init ((HttpRoute *) le->data);
		}
	}

}

#pragma endregion

#pragma region routes

void http_cerver_route_register (HttpCerver *http_cerver, HttpRoute *route) {

	if (http_cerver && route) {
		dlist_insert_after (http_cerver->routes, dlist_end (http_cerver->routes), route);
	}

}

void http_cerver_set_catch_all_route (HttpCerver *http_cerver, 
	void (*catch_all_route)(CerverReceive *cr, HttpRequest *request)) {

	if (http_cerver && catch_all_route) {
		http_cerver->default_handler = catch_all_route;
	}

}

#pragma endregion

#pragma region handler

static int http_receive_handle_url (http_parser *parser, const char *at, size_t length) {

	// printf ("\n%s\n\n", at);
	// printf ("%.*s\n", (int) length, at);

	HttpRequest *request = (HttpRequest *) parser->data;
	request->url = estring_new (NULL);
	request->url->str = c_string_create ("%.*s", (int) length, at);
	request->url->len = length;

	return 0;

}

static inline int http_receive_handle_header_field_handle (const char *header) {

	if (!strcmp ("Accept", header)) return REQUEST_HEADER_ACCEPT;
	if (!strcmp ("Accept-Charset", header)) return REQUEST_HEADER_ACCEPT_CHARSET;
	if (!strcmp ("Accept-Encoding", header)) return REQUEST_HEADER_ACCEPT_ENCODING;
	if (!strcmp ("Accept-Language", header)) return REQUEST_HEADER_ACCEPT_LANGUAGE;

	if (!strcmp ("Access-Control-Request-Headers", header)) return REQUEST_HEADER_ACCESS_CONTROL_REQUEST_HEADERS;

	if (!strcmp ("Authorization", header)) return REQUEST_HEADER_AUTHORIZATION;

	if (!strcmp ("Cache-Control", header)) return REQUEST_HEADER_CACHE_CONTROL;

	if (!strcmp ("Connection", header)) return REQUEST_HEADER_CONNECTION;

	if (!strcmp ("Content-Length", header)) return REQUEST_HEADER_CONTENT_LENGTH;
	if (!strcmp ("Content-Type", header)) return REQUEST_HEADER_CONTENT_TYPE;

	if (!strcmp ("Cookie", header)) return REQUEST_HEADER_COOKIE;

	if (!strcmp ("Date", header)) return REQUEST_HEADER_DATE;

	if (!strcmp ("Expect", header)) return REQUEST_HEADER_EXPECT;

	if (!strcmp ("Host", header)) return REQUEST_HEADER_HOST;

	if (!strcmp ("Proxy-Authorization", header)) return REQUEST_HEADER_PROXY_AUTHORIZATION;

	if (!strcmp ("User-Agent", header)) return REQUEST_HEADER_USER_AGENT;

	return REQUEST_HEADER_INVALID;		// no known header

}

static int http_receive_handle_header_field (http_parser *parser, const char *at, size_t length) {

	char header[32] = { 0 };
	snprintf (header, 32, "%.*s", (int) length, at);
	// printf ("\nheader field: /%s/\n", header);

	((HttpRequest *) parser->data)->next_header = http_receive_handle_header_field_handle (header);

	return 0;

}

static int http_receive_handle_header_value (http_parser *parser, const char *at, size_t length) {

	// printf ("\nheader value: %s\n", at);

	HttpRequest *request = (HttpRequest *) parser->data;
	if (request->next_header != REQUEST_HEADER_INVALID) {
		request->headers[request->next_header] = estring_new (NULL);
		request->headers[request->next_header]->str = c_string_create ("%.*s", (int) length, at);
		request->headers[request->next_header]->len = length;
	}

	// request->next_header = REQUEST_HEADER_INVALID;

	return 0;

}

static int http_receive_handle_body (http_parser *parser, const char *at, size_t length) {

	// printf ("Body: %.*s", (int) length, at);

	HttpRequest *request = (HttpRequest *) parser->data;
	request->body = estring_new (NULL);
	request->body->str = c_string_create ("%.*s", (int) length, at);
	request->body->len = length;

	return 0;

}

static void http_receive_handle_default_route (CerverReceive *cr, HttpRequest *request) {

	// FIXME:

}

// select the route that will handle the request
static void http_receive_handle_select (CerverReceive *cr, HttpRequest *request) {

	HttpCerver *http_cerver = (HttpCerver *) cr->cerver->cerver_data;

	// split the real url variables from the query
	// TODO:

	bool match = false;

	size_t n_tokens = 0;
	char **tokens = NULL;

	// search top level routes at the start of the url
	HttpRoute *route = NULL;
	ListElement *le = dlist_start (http_cerver->routes);
	while (!match && le) {
		route = (HttpRoute *) le->data;

		if (route->route->len == request->url->len) {
			if (!strcmp (route->route->str, request->url->str)) {
				// we have a direct match
				match = true;
				route->handler (cr, request);
				break;
			}
		}

		else {
			if (route->children->size) {
				printf ("request->url->str: %s\n", request->url->str);
				char *start_sub = strstr (request->url->str, route->route->str);
				if (start_sub) {
					// match and still some path left
					char *end_sub = request->url->str + route->route->len;

					printf ("first end_sub: %s\n", end_sub);

					tokens = c_string_split (end_sub, '/', &n_tokens);

					printf ("n tokens: %ld\n", n_tokens);
					printf ("second end_sub: %s\n", end_sub);

					if (tokens) {
						HttpRoutesTokens *routes_tokens = route->routes_tokens[n_tokens - 1];
						if (routes_tokens->n_routes) {
							// match all url tokens with existing route tokens
							bool fail = false;
							for (unsigned int main_idx = 0; main_idx < routes_tokens->n_routes; main_idx++) {
								// printf ("testing route %s\n", routes_tokens->routes[main_idx]->route->str);

								if (routes_tokens->tokens[main_idx]) {
									fail = false;
									for (unsigned int sub_idx = 0; sub_idx < routes_tokens->id; sub_idx++) {
										if (strcmp (routes_tokens->tokens[main_idx][sub_idx], tokens[sub_idx])) {
											fail = true;
											break;
										}
									}

									if (!fail) {
										// we have found our route!
										match = true;
										routes_tokens->routes[main_idx]->handler (cr, request);
										break;
									}
								}
							}
						}

						for (size_t i = 0; i < n_tokens; i++) if (tokens[i]) free (tokens[i]);
						free (tokens);
						tokens = NULL;
					}
				}
			}
		}

		le = le->next;
	}

	// catch all mismatches and handle with user defined route
	if (!match) {
		char *status = c_string_create ("No matching route for url %s", request->url->str);
		if (status) {
			cerver_log_warning (status);
			free (status);
		}

		// handle with default route
		http_cerver->default_handler (cr, request);
	}

}

void http_receive_handle (CerverReceive *cr, ssize_t rc, char *packet_buffer) {

	http_parser *parser = malloc (sizeof (http_parser));
	http_parser_init (parser, HTTP_REQUEST);

	http_parser_settings settings = { 0 };
	settings.on_message_begin = NULL;
	settings.on_url = http_receive_handle_url;
	settings.on_status = NULL;
	settings.on_header_field = http_receive_handle_header_field;
	settings.on_header_value = http_receive_handle_header_value;
	settings.on_headers_complete = NULL;
	settings.on_body = http_receive_handle_body;
	settings.on_message_complete = NULL;
	settings.on_chunk_header = NULL;
	settings.on_chunk_complete = NULL;

	HttpRequest *request = http_request_new ();
	parser->data = request;

	size_t n_parsed = http_parser_execute (parser, &settings, packet_buffer, rc);

	printf ("\nn parsed %ld / received %ld\n", n_parsed, rc);

	// printf ("Method: %s\n", http_method_str (parser->method));

	request->method = parser->method;

	// http_request_headers_print (request);

	// select method handler
	http_receive_handle_select (cr, request);

	http_request_delete (request);

	connection_end (cr->connection);

	// packet_delete (packet);
	// estring_delete (response);

	free (parser);

	printf ("http_receive_handle () - end!\n");

}

#pragma endregion