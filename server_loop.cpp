#include "server.hpp"

void server_loop(ServerEnv *env)
{
    // Each structure monitors one file descriptor
    struct pollfd fds[MAX_CLIENT + 1];
    std::memset(fds, 0, sizeof(fds));

    fds[0].fd = env->getServerFd();

    fds[0].events = POLLIN; // We want to know if someone tries to connect.
    
    int client_count = 0;

    while(true)
    {
        //wait for event on any fd. 
        int activity = poll(fds, client_count + 1, -1); // -1 = wait forever.
        
        if(activity < 0)
        {
            std::cerr << "Error: poll() failed" << std::endl;
            continue;
        }

        //check if there is an activity on the server socket(new connection)
        //events: These are the events I'm interested in for this fd. Events we want to watch (POLLIN = readable)
        //revents: These are the events that really occurred on this fd since the last poll().
        
        if(fds[0].revents & POLLIN) //handles new connections
        {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(env->getServerFd(), (struct sockaddr *)&client_addr, &addr_len);

            if(client_fd == -1)
            {
                std::cerr << "Error: accept() failed!" << std::endl;
                continue;
            }

            if(client_count >= MAX_CLIENT)
            {
                std::cerr << "Error: Max clients reached!" << std::endl;
                send_message(client_fd, "Error: server is full\r\n");
                close(client_fd);
                continue;
            }

            set_non_blocking(client_fd);

            fds[++client_count].fd = client_fd;
            fds[client_count].events = POLLIN;

            Client new_client;
            new_client.setHostname(inet_ntoa(client_addr.sin_addr));
            env->addClient(client_fd, new_client);

            std::cout << "New Client Connected: fd=" << client_fd
                      << " from " << inet_ntoa(client_addr.sin_addr)
                      << ":" << ntohs(client_addr.sin_port) << std::endl;
        }


        for(int i = 1; i <= client_count; i++)
        {
            if(fds[i].revents & POLLIN)
            {
                int client_fd = fds[i].fd;

                // Check if client still exists
                if(!env->hasClient(client_fd))
                {
                    // Remove from fds array by shifting
                    for(int j = i; j < client_count; j++)
                        fds[j] = fds[j + 1];
                    client_count--;
                    i--;
                    continue;
                }

                handle_client_data(client_fd, env);
                
                if(!env->hasClient(client_fd))
                {
                    for(int j = i; j < client_count; j++)
                        fds[j] = fds[j + 1];
                    client_count--;
                    i--;
                }
            }
        }
    }
}

void cleanup_server(ServerEnv *env)
{
    // Close all client connections
    std::map<int, Client> &clients = env->getClients();
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        close(it->first);
    }
    clients.clear();
    
    // Clear all channels
    std::map<std::string, Channel> &channels = env->getChannels();
    channels.clear();
    
    // Close server socket
    if (env->getServerFd() != -1)
    {
        close(env->getServerFd());
        env->setServerFd(-1);
    }
}