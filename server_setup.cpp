#include "server.hpp"

int create_server_socket(int port)
{
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "Error: Failed to create socket!" << std::endl;
        exit(1);
    }

    //allows reusing the address(socket) immediately after server stops
    int option = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        std::cerr << "Error: setsockop failed!" << std::endl;
        close(server_fd);
        exit(1);
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr)); // fill all the data with 0 removing garbage.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept from any IP
    server_addr.sin_port = htons(port); // Convert port to network byte order format (Host To Network)

    if(bind(server_fd,(struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "Error: Failed to bind socket to port:" << port << "!" << std::endl;
        std::cerr  << "Make sure the port is not already in use." << std::endl;
        close(server_fd);
        exit(1);
    }

    if(listen(server_fd, MAX_CLIENT) == -1)
    {
        std::cerr << "Error: Failed to listen on socket!" << std::endl;
        close(server_fd);
        exit(1);
    }

    set_non_blocking(server_fd);
    return server_fd;
}

void set_non_blocking(int fd)
{
    if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Error: fcntl F_SETFL failed!" << std::endl;
        exit(1);
    }
}