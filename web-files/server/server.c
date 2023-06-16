#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PORT 5000

struct connection_info
{
    char *response;
    size_t length;
};

static int handle_login_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls);

static int handle_upload_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls);

static int handle_get_file_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls);

int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                 const char *filename, const char *content_type,
                 const char *transfer_encoding, const char *data, uint64_t off, size_t size)
{
    printf("Received file data: %.*s\n", (int)size, data);

    char filepath[256];
    sprintf(filepath, "copied-files/%s", filename);
    FILE *file = fopen(filepath, "wb");
    if (file == NULL)
    {
        return MHD_NO;
    }

    size_t bytes_written = fwrite(data, sizeof(char), size, file);
    fclose(file);

    if (bytes_written != size)
    {
        return MHD_NO;
    }

    printf("File '%s' created and data written successfully!\n", filename);

    return MHD_YES;
}

static int handle_login_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (*con_cls == NULL)
    {
        const char *response = "{\"message\":\"Login successful!\"}";
        size_t response_length = strlen(response);

        struct connection_info *connection_info = malloc(sizeof(struct connection_info));
        if (connection_info == NULL)
        {
            return MHD_NO;
        }

        connection_info->response = malloc(response_length + 1);
        if (connection_info->response == NULL)
        {
            free(connection_info);
            return MHD_NO;
        }

        memcpy(connection_info->response, response, response_length);
        connection_info->response[response_length] = '\0';
        connection_info->length = response_length;

        *con_cls = connection_info;

        struct MHD_Response *http_response = MHD_create_response_from_buffer(
            connection_info->length, connection_info->response, MHD_RESPMEM_MUST_COPY);
        if (http_response == NULL)
        {
            free(connection_info->response);
            free(connection_info);
            return MHD_NO;
        }

        MHD_add_response_header(http_response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(http_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(http_response, "Access-Control-Allow-Headers", "Content-Type");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, http_response);
        MHD_destroy_response(http_response);
        return ret;
    }
    else
    {
        struct connection_info *connection_info = *con_cls;
        free(connection_info->response);
        free(connection_info);
        *con_cls = NULL;
        return MHD_NO;
    }
}

static int handle_upload_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (strcmp(method, "POST") == 0)
    {
        struct MHD_PostProcessor *pp = (struct MHD_PostProcessor *)*con_cls;
        if (*con_cls == NULL)
        {
            *con_cls = MHD_create_post_processor(connection, 1024, &iterate_post, NULL);
            if (*con_cls == NULL)
                return MHD_NO;

            return MHD_YES;
        }

        if (*upload_data_size > 0)
        {
            MHD_post_process(*con_cls, upload_data, *upload_data_size);

            *upload_data_size = 0;
            return MHD_YES;
        }
        else
        {
            MHD_destroy_post_processor((struct MHD_PostProcessor *)*con_cls);
            *con_cls = NULL;

            const char *response = "{\"message\":\"File uploaded successfully!\"}";
            size_t response_length = strlen(response);

            struct MHD_Response *http_response = MHD_create_response_from_buffer(
                response_length, (void *)response, MHD_RESPMEM_PERSISTENT);
            if (http_response == NULL)
                return MHD_NO;

            MHD_add_response_header(http_response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(http_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            MHD_add_response_header(http_response, "Access-Control-Allow-Headers", "Content-Type");

            int ret = MHD_queue_response(connection, MHD_HTTP_OK, http_response);
            MHD_destroy_response(http_response);
            return ret;
        }
    }
    else
    {
        return MHD_NO;
    }
}

static int handle_get_file_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (strcmp(method, "GET") == 0)
    {
        const char *filename = "";

        const char *filename_start = strchr(url, '?');
        if (filename_start != NULL)
        {
            filename_start += 1;
            const char *filename_end = strchr(filename_start, '=');
            if (filename_end != NULL)
            {
                filename_end += 1;
                size_t filename_length = filename_end - filename_start;
                char extracted_filename[256];
                strncpy(extracted_filename, filename_start, filename_length);
                extracted_filename[filename_length] = '\0';
                filename = extracted_filename;
            }
        }

        char filepath[256];
        sprintf(filepath, "copied-files/%s", filename);

        if (access(filepath, F_OK) == -1)
        {
            return MHD_NO;
        }

        FILE *file = fopen(filepath, "rb");
        if (file == NULL)
        {
            return MHD_NO;
        }

        struct stat st;
        if (stat(filepath, &st) != 0)
        {
            fclose(file);
            return MHD_NO;
        }

        size_t file_size = st.st_size;

        char *file_data = malloc(file_size);
        if (file_data == NULL)
        {
            fclose(file);
            return MHD_NO;
        }

        size_t bytes_read = fread(file_data, sizeof(char), file_size, file);
        fclose(file);

        if (bytes_read != file_size)
        {
            free(file_data);
            return MHD_NO;
        }

        struct MHD_Response *http_response = MHD_create_response_from_buffer(
            file_size, file_data, MHD_RESPMEM_MUST_FREE);
        if (http_response == NULL)
        {
            free(file_data);
            return MHD_NO;
        }

        MHD_add_response_header(http_response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(http_response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        MHD_add_response_header(http_response, "Access-Control-Allow-Headers", "Content-Type");

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, http_response);
        MHD_destroy_response(http_response);
        return ret;
    }
    else
    {
        return MHD_NO;
    }
}

static int handle_request(
    void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **con_cls)
{
    if (strcmp(method, "GET") == 0 && strcmp(url, "/getfile") == 0)
    {
        return handle_get_file_request(cls, connection, url, method, version, upload_data, upload_data_size, con_cls);
    }
    else if (strcmp(url, "/api/login") == 0)
    {

        return handle_login_request(cls, connection, url, method, version, upload_data, upload_data_size, con_cls);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(url, "/upload") == 0)
    {

        return handle_upload_request(cls, connection, url, method, version, upload_data, upload_data_size, con_cls);
    }
    else
    {
        return MHD_NO;
    }
}

int main()
{
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG, PORT, NULL, NULL,
        &handle_request, NULL, MHD_OPTION_END);

    if (daemon == NULL)
    {
        printf("Error starting the server.\n");
        return 1;
    }

    printf("Server running on port %d...\n", PORT);

    getchar();

    MHD_stop_daemon(daemon);

    return 0;
}
