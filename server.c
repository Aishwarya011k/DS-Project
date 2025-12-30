/* Simple single-threaded HTTP server in C
   Serves index.html and provides simple API endpoints:
   - GET /products         -> JSON list of products
   - GET /add?id=ID&qty=Q  -> add quantity to cart
   - GET /remove?id=ID     -> remove item from cart
   - GET /cart             -> JSON cart
   - GET /checkout         -> perform checkout and return bill

   Build on Windows (MinGW):
     gcc server.c -o server.exe -lws2_32

*/

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define close_socket closesocket
#pragma comment(lib, "ws2_32")
#ifdef _WIN32
typedef int socklen_t;
#endif
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define close_socket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080
#define BACKLOG 10
#define BUF_SIZE 8192
#define MAX_PRODUCTS 128

/* Product and Cart structures (simple arrays/linked list) */
struct Product {
    int id;
    char name[64];
    double price;
    int stock;
};

struct CartItem {
    int productId;
    char name[64];
    double price;
    int quantity;
    struct CartItem *next;
};

struct Product products[MAX_PRODUCTS];
int productCount = 0;
struct CartItem *cartHead = NULL;

void seedProducts() {
    // sample products (IDs 1..12 to match frontend)
    productCount = 12;
    int ids[] = {1,2,3,4,5,6,7,8,9,10,11,12};
    const char *names[] = {"Laptop","Smartphone","Headphones","Camera","Watch","Keyboard","Mouse","Monitor","Tablet","Speakers","Webcam","Microphone"};
    double prices[] = {999.99,699.99,199.99,799.99,299.99,149.99,49.99,399.99,549.99,179.99,89.99,129.99};
    int stocks[] = {5,10,15,7,12,20,30,6,8,14,25,18};
    for (int i = 0; i < productCount; ++i) {
        products[i].id = ids[i];
        strncpy(products[i].name, names[i], sizeof(products[i].name)-1);
        products[i].price = prices[i];
        products[i].stock = stocks[i];
    }
}

struct Product* findProduct(int id) {
    for (int i = 0; i < productCount; ++i) {
        if (products[i].id == id) return &products[i];
    }
    return NULL;
}

void add_to_cart(int id, int qty, char *out, size_t outlen) {
    struct Product *p = findProduct(id);
    if (!p) {
        snprintf(out, outlen, "{ \"error\": \"Product not found\" }");
        return;
    }
    if (qty > p->stock) {
        snprintf(out, outlen, "{ \"error\": \"Insufficient stock\" }");
        return;
    }
    struct CartItem *it = cartHead;
    while (it) {
        if (it->productId == id) {
            if (it->quantity + qty > p->stock) {
                snprintf(out, outlen, "{ \"error\": \"Exceeds stock\" }");
                return;
            }
            it->quantity += qty;
            snprintf(out, outlen, "{ \"status\": \"updated\" }");
            return;
        }
        it = it->next;
    }
    struct CartItem *n = malloc(sizeof(*n));
    n->productId = p->id;
    strncpy(n->name, p->name, sizeof(n->name)-1);
    n->price = p->price;
    n->quantity = qty;
    n->next = cartHead;
    cartHead = n;
    snprintf(out, outlen, "{ \"status\": \"added\" }");
}

void remove_from_cart(int id, char *out, size_t outlen) {
    struct CartItem *it = cartHead, *prev = NULL;
    while (it) {
        if (it->productId == id) {
            if (prev) prev->next = it->next; else cartHead = it->next;
            free(it);
            snprintf(out, outlen, "{ \"status\": \"removed\" }");
            return;
        }
        prev = it; it = it->next;
    }
    snprintf(out, outlen, "{ \"error\": \"Not in cart\" }");
}

void cart_json(char *out, size_t outlen) {
    char buf[4096];
    size_t pos = 0;
    pos += snprintf(out+pos, outlen-pos, "{");
    // items
    pos += snprintf(out+pos, outlen-pos, "\"items\": [");
    struct CartItem *it = cartHead; int first = 1;
    while (it) {
        if (!first) pos += snprintf(out+pos, outlen-pos, ",");
        pos += snprintf(out+pos, outlen-pos, "{\"id\":%d,\"name\":\"%s\",\"price\":%.2f,\"quantity\":%d}", it->productId, it->name, it->price, it->quantity);
        first = 0; it = it->next;
    }
    pos += snprintf(out+pos, outlen-pos, "],");
    double total = 0; it = cartHead;
    while (it) { total += it->price * it->quantity; it = it->next; }
    pos += snprintf(out+pos, outlen-pos, "\"total\": %.2f", total);
    pos += snprintf(out+pos, outlen-pos, "}");
}

void checkout_json(char *out, size_t outlen) {
    if (!cartHead) {
        snprintf(out, outlen, "{ \"error\": \"Cart empty\" }");
        return;
    }
    double total = 0; struct CartItem *it = cartHead;
    while (it) {
        struct Product *p = findProduct(it->productId);
        if (p) p->stock -= it->quantity;
        total += it->price * it->quantity;
        it = it->next;
    }
    // clear cart
    while (cartHead) {
        struct CartItem *t = cartHead; cartHead = cartHead->next; free(t);
    }
    snprintf(out, outlen, "{ \"status\": \"checked_out\", \"total\": %.2f }", total);
}

void products_json(char *out, size_t outlen) {
    size_t pos = 0;
    pos += snprintf(out+pos, outlen-pos, "[");
    for (int i = 0; i < productCount; ++i) {
        if (i) pos += snprintf(out+pos, outlen-pos, ",");
        pos += snprintf(out+pos, outlen-pos, "{\"id\":%d,\"name\":\"%s\",\"price\":%.2f,\"emoji\":\"%s\",\"category\":\"%s\"}",
            products[i].id, products[i].name, products[i].price,
            (i==0?"💻": (i==1?"📱": (i==2?"🎧": (i==3?"📷": (i==4?"⌚": (i==5?"⌨️": (i==6?"🖱️": (i==7?"🖥️": (i==8?"📱": (i==9?"🔊": (i==10?"📹":"🎤"))))))))))),
            (i==2||i==9||i==11?"audio": (i==3||i==10?"cameras": (i==4||i==5||i==6?"accessories":"electronics"))));
    }
    pos += snprintf(out+pos, outlen-pos, "]");
}

// Helper: serve file content (index.html)
int serve_file(const char *path, char *resp, size_t resp_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz+1);
    fread(buf,1,sz,f);
    buf[sz]=0;
    snprintf(resp, resp_len, "%s", buf);
    free(buf);
    fclose(f);
    return 0;
}

// Parse query string like id=1&qty=2
void parse_query(const char *qs, int *id, int *qty) {
    *id = -1; *qty = 1;
    if (!qs) return;
    char *copy = strdup(qs);
    char *token = strtok(copy, "&");
    while (token) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = 0; char *k = token; char *v = eq+1;
            if (strcmp(k, "id") == 0) *id = atoi(v);
            if (strcmp(k, "qty") == 0) *qty = atoi(v);
        }
        token = strtok(NULL, "&");
    }
    free(copy);
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    seedProducts();

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(server_fd, BACKLOG) < 0) { perror("listen"); return 1; }

    printf("Server listening on http://0.0.0.0:%d/\n", PORT);

    while (1) {
        struct sockaddr_in client;
        socklen_t clilen = sizeof(client);
        socket_t client_fd = accept(server_fd, (struct sockaddr*)&client, &clilen);
        if (client_fd < 0) { perror("accept"); continue; }

        char buf[BUF_SIZE];
        int r = recv(client_fd, buf, sizeof(buf)-1, 0);
        if (r <= 0) { close_socket(client_fd); continue; }
        buf[r]=0;

        // parse request line
        char method[16], path[1024];
        sscanf(buf, "%15s %1023s", method, path);

        // split path and query
        char *q = strchr(path, '?');
        char query[512] = {0};
        if (q) { strncpy(query, q+1, sizeof(query)-1); *q = 0; }

        char response[16384];
        char body[8192] = {0};
        char header[512] = {0};

        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            if (serve_file("index.html", body, sizeof(body)) == 0) {
                snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %zu\r\n\r\n", strlen(body));
                send(client_fd, header, strlen(header), 0);
                send(client_fd, body, strlen(body), 0);
            } else {
                const char *notf = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
                send(client_fd, notf, strlen(notf), 0);
            }
        }
        else if (strcmp(path, "/products") == 0) {
            products_json(body, sizeof(body));
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %zu\r\n\r\n", strlen(body));
            send(client_fd, header, strlen(header), 0);
            send(client_fd, body, strlen(body), 0);
        }
        else if (strcmp(path, "/add") == 0) {
            int id, qty; parse_query(query, &id, &qty);
            add_to_cart(id, qty, body, sizeof(body));
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n", strlen(body));
            send(client_fd, header, strlen(header), 0);
            send(client_fd, body, strlen(body), 0);
        }
        else if (strcmp(path, "/remove") == 0) {
            int id, qty; parse_query(query, &id, &qty);
            remove_from_cart(id, body, sizeof(body));
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n", strlen(body));
            send(client_fd, header, strlen(header), 0);
            send(client_fd, body, strlen(body), 0);
        }
        else if (strcmp(path, "/cart") == 0) {
            cart_json(body, sizeof(body));
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n", strlen(body));
            send(client_fd, header, strlen(header), 0);
            send(client_fd, body, strlen(body), 0);
        }
        else if (strcmp(path, "/checkout") == 0) {
            checkout_json(body, sizeof(body));
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n", strlen(body));
            send(client_fd, header, strlen(header), 0);
            send(client_fd, body, strlen(body), 0);
        }
        else {
            const char *notf = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
            send(client_fd, notf, strlen(notf), 0);
        }

        close_socket(client_fd);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
