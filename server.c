#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS] = {0}, max_sd, activity, valread;
    struct sockaddr_in address;
    char buffer[1024];
    fd_set readfds;
    socklen_t addrlen = sizeof(address);  // Add this line
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Bind to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT);
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        
        // Add clients to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0)
                FD_SET(client_sockets[i], &readfds);
            if (client_sockets[i] > max_sd)
                max_sd = client_sockets[i];
        }
        
        // Wait for activity
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }
        
        // Check for new connections
        if (FD_ISSET(server_fd, &readfds)) {
            socklen_t addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("Accept failed");
                continue;
            }
            printf("New connection: socket %d, IP %s, Port %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            
            // Add to client sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        
        // Check for incoming messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, sizeof(buffer) - 1)) == 0) {
                    getpeername(sd, (struct sockaddr *)&address, &addrlen);
                    printf("Client disconnected: IP %s, Port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Client %d: %s\n", sd, buffer);
                }
            }
        }
    }
    return 0;
}
