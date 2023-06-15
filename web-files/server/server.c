#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 5000

struct connection_info
{
    char *response;
    size_t length;
};

static int handle_login_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (*con_cls == NULL)
    {
        // This is the first call, allocate response and store it in con_cls
        const char *response = "{\"message\":\"Login successful!\"}";
        size_t response_length = strlen(response);

        struct connection_info *connection_info = malloc(sizeof(struct connection_info));
        if (connection_info == NULL)
        {
            return MHD_NO; // Returning MHD_NO to close the connection
        }

        connection_info->response = malloc(response_length + 1);
        if (connection_info->response == NULL)
        {
            free(connection_info);
            return MHD_NO; // Returning MHD_NO to close the connection
        }

        memcpy(connection_info->response, response, response_length);
        connection_info->response[response_length] = '\0'; // Null-terminate the response string
        connection_info->length = response_length;

        *con_cls = connection_info;

        // Respond with the stored response
        struct MHD_Response *http_response = MHD_create_response_from_buffer(
            connection_info->length, connection_info->response, MHD_RESPMEM_MUST_COPY);
        if (http_response == NULL)
        {
            free(connection_info->response);
            free(connection_info);
            return MHD_NO; // Returning MHD_NO to close the connection
        }

        // Add CORS headers to the response
        MHD_add_response_header(http_response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(http_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(http_response, "Access-Control-Allow-Headers", "Content-Type");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, http_response);
        MHD_destroy_response(http_response);
        return ret;
    }
    else
    {
        // This is a subsequent call, response has already been sent
        struct connection_info *connection_info = *con_cls;
        free(connection_info->response);
        free(connection_info);
        *con_cls = NULL;
        return MHD_NO; // Returning MHD_NO to close the connection
    }
}

static int handle_upload_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (strcmp(method, "POST") == 0)
    {
        // Retrieve the file name from the request body
        char filename[256];
        if (*upload_data_size >= sizeof(filename))
            return MHD_NO;

        memcpy(filename, upload_data, *upload_data_size);
        filename[*upload_data_size] = '\0';

        // Perform necessary operations to process the uploaded file
        // Here, we simply return a sample response with the file name
        const char *response_format = "{\"filename\":\"%s\"}";
        size_t response_length = snprintf(NULL, 0, response_format, filename);
        char *response = malloc(response_length + 1);
        if (response == NULL)
            return MHD_NO;

        snprintf(response, response_length + 1, response_format, filename);

        struct MHD_Response *http_response = MHD_create_response_from_buffer(
            response_length, response, MHD_RESPMEM_MUST_COPY);
        if (http_response == NULL)
        {
            free(response);
            return MHD_NO;
        }

        MHD_add_response_header(http_response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(http_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(http_response, "Access-Control-Allow-Headers", "Content-Type");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, http_response);
        MHD_destroy_response(http_response);
        free(response);
        return ret;
    }
    else
    {
        // Unsupported method
        return MHD_NO;
    }
}

static int handle_get_file_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (strcmp(method, "POST") == 0)
    {
        // Retrieve the file name from the request body
        char filename[256];
        if (*upload_data_size >= sizeof(filename))
            return MHD_NO;

        memcpy(filename, upload_data, *upload_data_size);
        filename[*upload_data_size] = '\0';

        // Perform necessary operations to retrieve the selected file
        // Here, we simply return a sample response with the file name
        const char *response_format = "{\"filename\":\"%s\"}";
        size_t response_length = snprintf(NULL, 0, response_format, filename);
        char *response = malloc(response_length + 1);
        if (response == NULL)
            return MHD_NO;

        snprintf(response, response_length + 1, response_format, filename);

        struct MHD_Response *http_response = MHD_create_response_from_buffer(
            response_length, response, MHD_RESPMEM_MUST_COPY);
        if (http_response == NULL)
        {
            free(response);
            return MHD_NO;
        }

        MHD_add_response_header(http_response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(http_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(http_response, "Access-Control-Allow-Headers", "Content-Type");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, http_response);
        MHD_destroy_response(http_response);
        free(response);
        return ret;
    }
    else
    {
        // Unsupported method
        return MHD_NO;
    }
}

int main()
{
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION, PORT, NULL, NULL,
        &handle_login_request, NULL, MHD_OPTION_END);

    if (daemon == NULL)
    {
        printf("Failed to start the server.\n");
        return 1;
    }

    printf("Server is running on port %d\n", PORT);
    getchar(); // Wait for user input to stop the server

    MHD_stop_daemon(daemon);
    return 0;
}
