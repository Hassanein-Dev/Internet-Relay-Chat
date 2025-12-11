#include "server.hpp"

void handle_client_data(int client_fd, ServerEnv *env)
{
    // buffer for receiving data
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0)
    {
        if (bytes_received == 0)
        {
            std::cout << "Client " << client_fd - 3 << " disconnected." << std::endl;
            disconnect_client(client_fd, env);
            return;
        }
        else
        {
            // if no data available
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            std::cerr << "Error: recv() failed for client " << client_fd << std::endl;
            disconnect_client(client_fd, env);
            return;
        }
    }

    env->getClient(client_fd).appendBuffer(std::string(buffer, bytes_received));

    std::string &client_buffer = env->getClient(client_fd).getBufferRef();
    size_t pos;

    while ((pos = client_buffer.find('\n')) != std::string::npos)
    {
        std::string command = client_buffer.substr(0, pos);
        client_buffer.erase(0, pos + 1);

        if (!command.empty() && command[command.length() - 1] == '\r') //\r returns the cursor to the fist index of the line;
            command.erase(command.length() - 1);

        if (command.empty())
            continue;

        std::cout << "Client " << client_fd - 3 << ": " << command << std::endl;        
        
        std::string cmd_name;
        
        size_t space_pos = command.find(' ');
        if (space_pos != std::string::npos)
            cmd_name = command.substr(0, space_pos);
        else
            cmd_name = command;

        std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::toupper);

        if (cmd_name == "CAP")
        {
            send_message(client_fd, "CAP * LS :\r\n"); // * : wildcard, versions, ls: list
            send_message(client_fd, "CAP * ACK :\r\n");
        }
        else if (cmd_name == "PASS")
        {
            handle_pass(client_fd, command, env);
            if (!env->hasClient(client_fd))
                return;
        }
        else if (cmd_name == "NICK")
            handle_nick(client_fd, command, env);
        else if (cmd_name == "USER")
            handle_user(client_fd, command, env);
        else if (!env->hasClient(client_fd) || !env->getClient(client_fd).isAuthenticated())
            send_message(client_fd, "Error: You must authenticate first (PASS, NICK, USER)\r\n");
        else if (cmd_name == "JOIN")
            handle_join(client_fd, command, env);
        else if (cmd_name == "PRIVMSG")
            handle_privmsg(client_fd, command, env);
        else if (cmd_name == "KICK")
            handle_kick(client_fd, command, env);
        else if (cmd_name == "INVITE")
            handle_invite(client_fd, command, env);
        else if (cmd_name == "TOPIC")
            handle_topic(client_fd, command, env);
        else if (cmd_name == "MODE")
            handle_mode(client_fd, command, env);
        else if (cmd_name == "WHO")
            handle_who(client_fd, command, env);
        else if (cmd_name == "WHOIS")
            handle_whois(client_fd, command, env);
        else if (cmd_name == "SENDFILE")
            handle_sendfile(client_fd, command, env);        
        else if (cmd_name == "GETFILE")
            handle_getfile(client_fd, command, env);
        else if (cmd_name == "QUIT")
        {
            for (std::map<std::string, Channel>::iterator it = env->getChannels().begin();
                 it != env->getChannels().end(); ++it)
            {
                Channel &chan = it->second;
                if (chan.hasClient(client_fd) && chan.isAdmin(client_fd))
                {
                    if (chan.getAdmins().size() == 1 && chan.getClients().size() > 1)
                    {
                        send_message(client_fd, "Error: You are the only operator in " + it->first + ". Promote another user first.\r\n");
                        return;
                    }
                }
            }
            
            send_message(client_fd, "Goodbye!\r\n");
            disconnect_client(client_fd, env);
            return;
        }
        else
            send_message(client_fd, "Error: Unknown command\r\n");
    }
}

void disconnect_client(int client_fd, ServerEnv *env)
{
    // Get client info before removing
    std::string nickname;
    std::string username;
    std::string hostname;
    
    if (env->hasClient(client_fd))
    {
        Client &client = env->getClient(client_fd);
        nickname = client.getNickname();
        username = client.getUsername();
        hostname = client.getHostname();
    }

    for (std::map<std::string, Channel>::iterator it = env->getChannels().begin();
         it != env->getChannels().end(); ++it)
    {
        Channel &chan = it->second;

        // Only process if client is in this channel
        if (!chan.hasClient(client_fd))
            continue;

        // Send QUIT message to all users in the channel before removing
        if (!nickname.empty())
        {
            std::string quit_msg = ":" + nickname + "!~" + username + "@" + hostname + 
                                  " QUIT :Client disconnected\r\n";
            broadcast_to_channel(quit_msg, it->first, env, client_fd);
        }

        if (chan.isAdmin(client_fd) && chan.getAdmins().size() == 1 && chan.getClients().size() > 1)
        {
            const std::vector<int> &clients = chan.getClients();
            for (size_t i = 0; i < clients.size(); i++)
            {
                if (clients[i] != client_fd)
                {
                    chan.addAdmin(clients[i]);
                    if (env->hasClient(clients[i]))
                    {
                        std::string promote_msg = ":server MODE " + it->first + " +o " + 
                                                 env->getClient(clients[i]).getNickname() + "\r\n";
                        broadcast_to_channel(promote_msg, it->first, env);
                        std::cout << "Auto-promoted " << env->getClient(clients[i]).getNickname() 
                                 << " to operator in " << it->first << std::endl;
                    }
                    break;
                }
            }
        }

        chan.removeClient(client_fd);
        chan.removeAdmin(client_fd);
        chan.removeInvited(client_fd);
    }

    close(client_fd);
    env->removeClient(client_fd);
    std::cout << "Client " << client_fd - 3 << " disconnected and cleaned up." << std::endl;
}